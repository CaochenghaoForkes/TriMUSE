"""几何与网格配置的语义检查"""

from __future__ import annotations
from .geometry_mesh_config import GeometryMeshCard, UnitValue
from .units import Units

class GeometryMeshExamine:
    """检查几何与网格配置是否具有一致的工程含义"""

    _SUPPORTED_SCHEMA_VERSIONS = frozenset({"0.1"})
    _SUPPORTED_SOURCES = frozenset({"ideal_multilayer_sphere", "stochastic_multilayer_particle"})
    _ELEMENTS_BY_DIMENSION = {
        1: frozenset({"line"}),
        2: frozenset({"triangle", "quadrilateral"}),
        3: frozenset({"tetrahedron", "hexahedron", "prism", "pyramid"}),
    }
    _SUPPORTED_ENGINES = frozenset({"gmsh"})
    _SUPPORTED_GMSH_ALGORITHMS_3D = frozenset({"hxt", "mmg3d", "del3d", "front3d"})
    _SUPPORTED_GMSH_OPTIMIZERS = frozenset({"gmsh", "netgen", "none"})
    _SUPPORTED_MSH_VERSIONS = frozenset({"2.2", "4.1"})

    @classmethod
    def examine(cls, card: GeometryMeshCard) -> None:
        """检查完整几何与网格配置卡"""
        if not isinstance(card, GeometryMeshCard):
            raise TypeError("几何与网格检查对象必须是 GeometryMeshCard")
        cls.check_schema_version(card.schema_version)
        cls.check_model(card.model)
        cls.check_geometry(card.geometry, card.model.dimension)
        cls.check_mesh(card.mesh, card.model.dimension)
        cls.check_region_references(card.geometry, card.mesh)

    @staticmethod
    def check_schema_version(schema_version: str) -> None:
        """检查输入卡版本"""
        if schema_version not in GeometryMeshExamine._SUPPORTED_SCHEMA_VERSIONS:
            supported = ", ".join(sorted(GeometryMeshExamine._SUPPORTED_SCHEMA_VERSIONS))
            raise ValueError(f"不支持的 geometry_mesh schema_version: {schema_version!r}，支持 {supported}")

    @staticmethod
    def check_model(model: GeometryMeshCard.Model) -> None:
        """检查空间维数"""
        if not isinstance(model, GeometryMeshCard.Model):
            raise TypeError("model 必须是 GeometryMeshCard.Model")
        if model.dimension not in GeometryMeshExamine._ELEMENTS_BY_DIMENSION:
            raise ValueError(f"model dimension 必须为 1、2 或 3，实际为 {model.dimension!r}")

    @staticmethod
    def check_geometry(geometry: GeometryMeshCard.Geometry, dimension: int) -> None:
        """检查 source 与几何模型参数"""
        if not isinstance(geometry, GeometryMeshCard.Geometry):
            raise TypeError("geometry 必须是 GeometryMeshCard.Geometry")
        if geometry.source not in GeometryMeshExamine._SUPPORTED_SOURCES:
            supported = ", ".join(sorted(GeometryMeshExamine._SUPPORTED_SOURCES))
            raise ValueError(f"不支持的 geometry source: {geometry.source!r}，支持 {supported}")

        if geometry.source == "ideal_multilayer_sphere":
            sphere = geometry.ideal_multilayer_sphere
            if sphere is None:
                raise ValueError(
                    "geometry source 为 ideal_multilayer_sphere 时，"
                    "必须提供 ideal_multilayer_sphere 节点"
                )
            GeometryMeshExamine.check_ideal_multilayer_sphere(sphere, dimension)

        if geometry.source == "stochastic_multilayer_particle":
            GeometryMeshExamine.check_stochastic_multilayer_particle(geometry.stochastic_multilayer_particle, dimension)

    @classmethod
    def check_stochastic_multilayer_particle(cls, particle: GeometryMeshCard.Geometry.StochasticMultilayerParticle, dimension: int) -> None:
        """检查随机颗粒的统计输入与形状参数"""
        if not isinstance(particle, GeometryMeshCard.Geometry.StochasticMultilayerParticle):
            raise TypeError("必须提供 stochastic_multilayer_particle 配置")
        for name in ("x", "y", "z")[:dimension]:
            cls.check_length(getattr(particle.center, name), f"center {name}")
        surface = particle.outer_surface
        if cls.check_length(surface.batch_mean_radius, "batch_mean_radius") <= 0:
            raise ValueError("外表面 batch_mean_radius 必须大于零")
        if dimension > 1:
            shape = surface.target_circularity if dimension == 2 else surface.target_sphericity
            if shape is None or not 0 < Units.convert(shape.value, shape.unit, "[1]") <= 1:
                raise ValueError("目标圆形度或球形度必须在 (0,1] 内")
            expansion = surface.shape_expansion
            if expansion is None or type(expansion.max_degree) is not int or expansion.max_degree < 2:
                raise ValueError("max_degree 必须是大于等于 2 的整数")
            if not 0 < expansion.spectrum_decay < float("inf"):
                raise ValueError("spectrum_decay 必须是有限正数")
        if not particle.layers.layer:
            raise ValueError("随机颗粒至少需要核心层")
        names, boundaries = set(), set()
        for index, layer in enumerate(particle.layers.layer):
            if not layer.name or layer.name in names:
                raise ValueError(f"layer 名称为空或重复: {layer.name!r}")
            if not layer.outer_boundary.name or layer.outer_boundary.name in boundaries:
                raise ValueError(f"outer_boundary 名称为空或重复: {layer.outer_boundary.name!r}")
            names.add(layer.name)
            boundaries.add(layer.outer_boundary.name)
            mean_name, std_name = ("batch_mean_radius", "batch_radius_std") if index == 0 else ("batch_mean_thickness", "batch_thickness_std")
            if cls.check_length(getattr(layer, mean_name), f"{layer.name} {mean_name}") <= 0:
                raise ValueError(f"{layer.name} 平均尺寸必须大于零")
            if cls.check_length(getattr(layer, std_name), f"{layer.name} {std_name}") < 0:
                raise ValueError(f"{layer.name} 批次标准差不能为负")
            if index > 0 and dimension > 1:
                if cls.check_length(layer.within_particle_thickness_std, f"{layer.name} thickness_std") < 0:
                    raise ValueError(f"{layer.name} 局部厚度标准差不能为负")
                if cls.check_length(layer.within_particle_radius_std, f"{layer.name} radius_std") <= 0:
                    raise ValueError(f"{layer.name} 局部半径标准差必须大于零")
                coefficient = layer.unwrapping_coefficient
                if coefficient is None or not 0 <= Units.convert(coefficient.value, coefficient.unit, "[1]") <= 1:
                    raise ValueError(f"{layer.name} 解包系数必须在 [0,1] 内")

    @staticmethod
    def check_ideal_multilayer_sphere(
        sphere: GeometryMeshCard.Geometry.IdealMultilayerSphere,
        dimension: int,
    ) -> None:
        """检查同心多层球的坐标和层参数"""
        if not isinstance(sphere, GeometryMeshCard.Geometry.IdealMultilayerSphere):
            raise TypeError("ideal_multilayer_sphere 必须是对应的配置对象")

        coordinates = (sphere.center.x, sphere.center.y, sphere.center.z)[:dimension]
        coordinate_names = ("x", "y", "z")[:dimension]
        for name, coordinate in zip(coordinate_names, coordinates):
            GeometryMeshExamine.check_length(coordinate, f"center {name}")

        layers = sphere.layers.layer
        if not layers:
            raise ValueError("ideal_multilayer_sphere 至少需要一个 layer")

        layer_names: set[str] = set()
        boundary_names: set[str] = set()
        previous_radius: float | None = None
        for index, layer in enumerate(layers, start=1):
            context = f"layer #{index}"
            if not layer.name:
                raise ValueError(f"{context} 的 name 不能为空")
            if layer.name in layer_names:
                raise ValueError(f"layer 名称重复: {layer.name!r}")
            layer_names.add(layer.name)

            boundary_name = layer.outer_boundary.name
            if not boundary_name:
                raise ValueError(f"{context} 的 outer_boundary name 不能为空")
            if boundary_name in boundary_names:
                raise ValueError(f"outer_boundary 名称重复: {boundary_name!r}")
            boundary_names.add(boundary_name)

            radius = GeometryMeshExamine.check_length(layer.outer_radius, f"{context} outer_radius")
            if radius <= 0.0:
                raise ValueError(f"{context} 的 outer_radius 必须大于零")
            if previous_radius is not None and radius <= previous_radius:
                raise ValueError(
                    f"{context} 的 outer_radius 必须严格大于前一层半径"
                )
            previous_radius = radius

    @staticmethod
    def check_mesh(mesh: GeometryMeshCard.Mesh, dimension: int) -> None:
        """检查网格引擎、单元和尺寸控制参数"""
        if not isinstance(mesh, GeometryMeshCard.Mesh):
            raise TypeError("mesh 必须是 GeometryMeshCard.Mesh")
        if mesh.engine not in GeometryMeshExamine._SUPPORTED_ENGINES:
            supported = ", ".join(sorted(GeometryMeshExamine._SUPPORTED_ENGINES))
            raise ValueError(f"不支持的 mesh engine: {mesh.engine!r}，支持 {supported}")

        allowed_elements = GeometryMeshExamine._ELEMENTS_BY_DIMENSION[dimension]
        if mesh.element.type not in allowed_elements:
            supported = ", ".join(sorted(allowed_elements))
            raise ValueError(
                f"{dimension}D 模型不支持单元类型 {mesh.element.type!r}，支持 {supported}"
            )
        if mesh.element.geometry_order not in (1, 2):
            raise ValueError(
                "element geometry_order 必须为 1 或 2，"
                f"实际为 {mesh.element.geometry_order!r}"
            )

        GeometryMeshExamine.check_sizing(mesh.sizing)
        GeometryMeshExamine.check_output(mesh.output)
        GeometryMeshExamine.check_gmsh(mesh.gmsh, dimension)

    @staticmethod
    def check_sizing(sizing: GeometryMeshCard.Mesh.Sizing) -> None:
        """检查全局和区域网格尺寸"""
        if not isinstance(sizing, GeometryMeshCard.Mesh.Sizing):
            raise TypeError("sizing 必须是 GeometryMeshCard.Mesh.Sizing")

        global_sizing = sizing.global_
        minimum = None
        maximum = None
        if global_sizing.min_edge_length is not None:
            minimum = GeometryMeshExamine.check_length(
                global_sizing.min_edge_length,
                "global min_edge_length",
            )
            if minimum <= 0.0:
                raise ValueError("global min_edge_length 必须大于零")
        if global_sizing.max_edge_length is not None:
            maximum = GeometryMeshExamine.check_length(
                global_sizing.max_edge_length,
                "global max_edge_length",
            )
            if maximum <= 0.0:
                raise ValueError("global max_edge_length 必须大于零")
        if minimum is not None and maximum is not None and minimum > maximum:
            raise ValueError("global min_edge_length 不能大于 max_edge_length")

        region_names: set[str] = set()
        for index, region in enumerate(sizing.region, start=1):
            context = f"sizing region #{index}"
            if not region.name:
                raise ValueError(f"{context} 的 name 不能为空")
            if region.name in region_names:
                raise ValueError(f"sizing region 名称重复: {region.name!r}")
            region_names.add(region.name)
            target = GeometryMeshExamine.check_length(
                region.target_edge_length,
                f"{context} target_edge_length",
            )
            if target <= 0.0:
                raise ValueError(f"{context} 的 target_edge_length 必须大于零")

    @staticmethod
    def check_region_references(
        geometry: GeometryMeshCard.Geometry,
        mesh: GeometryMeshCard.Mesh,
    ) -> None:
        """检查尺寸控制区域引用已定义的材料层"""
        sphere = getattr(geometry, geometry.source)
        assert sphere is not None
        layer_names = {layer.name for layer in sphere.layers.layer}
        for region in mesh.sizing.region:
            if region.name not in layer_names:
                raise ValueError(
                    f"sizing region {region.name!r} 未对应任何已定义的 layer"
                )

    @staticmethod
    def check_output(output: GeometryMeshCard.Mesh.Output) -> None:
        """检查网格输出路径"""
        if not isinstance(output, GeometryMeshCard.Mesh.Output):
            raise TypeError("output 必须是 GeometryMeshCard.Mesh.Output")
        if not isinstance(output.file, str) or not output.file.strip():
            raise ValueError("mesh output file 必须是非空路径")
        if not isinstance(output.write_vtk, bool):
            raise TypeError("mesh output write_vtk 必须为 bool")

    @staticmethod
    def check_gmsh(gmsh: GeometryMeshCard.Mesh.Gmsh, dimension: int) -> None:
        """检查 Gmsh 参数"""
        if not isinstance(gmsh, GeometryMeshCard.Mesh.Gmsh):
            raise TypeError("gmsh 必须是 GeometryMeshCard.Mesh.Gmsh")
        if dimension == 3 and gmsh.algorithm_3d not in GeometryMeshExamine._SUPPORTED_GMSH_ALGORITHMS_3D:
            supported = ", ".join(sorted(GeometryMeshExamine._SUPPORTED_GMSH_ALGORITHMS_3D))
            raise ValueError(
                f"不支持的 Gmsh algorithm_3d: {gmsh.algorithm_3d!r}，支持 {supported}"
            )
        if gmsh.optimizer not in GeometryMeshExamine._SUPPORTED_GMSH_OPTIMIZERS:
            supported = ", ".join(sorted(GeometryMeshExamine._SUPPORTED_GMSH_OPTIMIZERS))
            raise ValueError(f"不支持的 Gmsh optimizer: {gmsh.optimizer!r}，支持 {supported}")
        if gmsh.msh_version not in GeometryMeshExamine._SUPPORTED_MSH_VERSIONS:
            supported = ", ".join(sorted(GeometryMeshExamine._SUPPORTED_MSH_VERSIONS))
            raise ValueError(f"不支持的 Gmsh msh_version: {gmsh.msh_version!r}，支持 {supported}")
        if not isinstance(gmsh.binary, bool):
            raise TypeError("gmsh binary 必须为 bool")

    @staticmethod
    def check_length(quantity: UnitValue, context: str) -> float:
        """检查物理量为有限长度并换算为米"""
        if not isinstance(quantity, UnitValue):
            raise TypeError(f"{context} 必须是 UnitValue")
        return Units.to_si(
            quantity.value,
            quantity.unit,
            "length",
            context=context,
        )
