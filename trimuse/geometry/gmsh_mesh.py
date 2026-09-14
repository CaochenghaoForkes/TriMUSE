"""各种几何构造方式共用的 Gmsh 网格流程"""

from pathlib import Path
import gmsh
from trimuse.input.geometry_mesh_config import GeometryMeshCard
from trimuse.geometry.topology import GeometryTopology
MeshConfig = GeometryMeshCard.Mesh

class GmshMesh:
    """根据几何拓扑和网格配置生成 Gmsh 网格"""

    _ELEMENTS_BY_DIMENSION = {1: "line", 2: "triangle", 3: "tetrahedron"}
    _ENTITY_LISTS = {1: "CurvesList", 2: "SurfacesList", 3: "VolumesList"}
    _ALGORITHMS_3D = {"del3d": 1, "front3d": 4, "mmg3d": 7, "hxt": 10}
    _OPTIMIZERS = {"gmsh": "", "netgen": "Netgen"}

    @classmethod
    def generate(cls, topology: GeometryTopology, config: MeshConfig, output_path: str | Path) -> Path:
        """生成网格并按配置写出网格文件"""
        cls._check_element(topology.dimension, config.element.type)
        cls._add_physical_groups(topology)
        cls._set_mesh_sizes(topology, config.sizing)
        cls._set_algorithm(topology.dimension, config.gmsh.algorithm_3d)
        gmsh.model.mesh.generate(topology.dimension)
        cls._optimize(topology.dimension, config.gmsh.optimizer)
        cls._set_order(config.element.geometry_order, config.gmsh.optimizer)
        cls._check_mesh(topology)
        return cls._write(output_path, config.output, config.gmsh)

    @classmethod
    def _check_element(cls, dimension: int, element_type: str) -> None:
        """检查当前阶段能够直接生成的单元类型"""
        expected = cls._ELEMENTS_BY_DIMENSION[dimension]
        if element_type != expected:
            raise NotImplementedError(
                f"当前 {dimension}D Gmsh 网格仅支持 {expected!r}, "
                f"尚未实现 {element_type!r} 所需的结构化或重组流程"
            )

    @staticmethod
    def _add_physical_groups(topology: GeometryTopology) -> None:
        """将材料区域和边界注册为 Gmsh 物理组"""
        groups = ((topology.dimension, topology.regions), (topology.dimension - 1, topology.boundaries))
        for dimension, named_entities in groups:
            for name, entity_tags in named_entities.items():
                if not entity_tags:
                    raise ValueError(f"物理组 {name!r} 不包含任何几何实体")
                group_tag = gmsh.model.addPhysicalGroup(dimension, entity_tags)
                gmsh.model.setPhysicalName(dimension, group_tag, name)

    @classmethod
    def _set_mesh_sizes(cls, topology: GeometryTopology, sizing: MeshConfig.Sizing) -> None:
        """设置全局网格尺寸范围和材料区域目标尺寸"""
        global_sizing = sizing.global_
        if global_sizing.min_edge_length is not None:
            gmsh.option.setNumber("Mesh.MeshSizeMin", global_sizing.min_edge_length.value)
        if global_sizing.max_edge_length is not None:
            gmsh.option.setNumber("Mesh.MeshSizeMax", global_sizing.max_edge_length.value)

        region_fields = []
        entity_list = cls._ENTITY_LISTS[topology.dimension]
        for region in sizing.region:
            if region.name not in topology.regions:
                raise ValueError(f"网格尺寸区域 {region.name!r} 不存在于几何拓扑中")
            value_field = gmsh.model.mesh.field.add("MathEval")
            gmsh.model.mesh.field.setString(value_field, "F", repr(region.target_edge_length.value))
            region_field = gmsh.model.mesh.field.add("Restrict")
            gmsh.model.mesh.field.setNumber(region_field, "InField", value_field)
            gmsh.model.mesh.field.setNumbers(region_field, entity_list, topology.regions[region.name])
            gmsh.model.mesh.field.setNumber(region_field, "IncludeBoundary", 1)
            region_fields.append(region_field)

        if not region_fields:
            return
        background_field = region_fields[0]
        if len(region_fields) > 1:
            background_field = gmsh.model.mesh.field.add("Min")
            gmsh.model.mesh.field.setNumbers(background_field, "FieldsList", region_fields)
        gmsh.model.mesh.field.setAsBackgroundMesh(background_field)

    @classmethod
    def _set_algorithm(cls, dimension: int, algorithm_3d: str) -> None:
        """设置三维非结构网格算法"""
        if dimension == 3:
            gmsh.option.setNumber("Mesh.Algorithm3D", cls._ALGORITHMS_3D[algorithm_3d])

    @classmethod
    def _optimize(cls, dimension: int, optimizer: str) -> None:
        """优化三维一次网格"""
        if dimension == 3 and optimizer != "none":
            gmsh.model.mesh.optimize(cls._OPTIMIZERS[optimizer])

    @staticmethod
    def _set_order(order: int, optimizer: str) -> None:
        """设置几何阶次并按需优化高阶网格"""
        if order == 1:
            return
        gmsh.model.mesh.setOrder(order)
        if optimizer != "none":
            gmsh.model.mesh.optimize("HighOrder")

    @staticmethod
    def _check_mesh(topology: GeometryTopology) -> None:
        """确认每个材料区域均已生成最高维单元"""
        for name, entity_tags in topology.regions.items():
            element_count = 0
            for entity_tag in entity_tags:
                _, element_tags, _ = gmsh.model.mesh.getElements(topology.dimension, entity_tag)
                element_count += sum(len(tags) for tags in element_tags)
            if element_count == 0:
                raise RuntimeError(f"材料区域 {name!r} 未生成 {topology.dimension}D 单元")

    @staticmethod
    def _write(output_path: str | Path, output: MeshConfig.Output, config: MeshConfig.Gmsh) -> Path:
        """设置输出格式并写出网格文件"""
        path = Path(output_path).expanduser().resolve()
        path.parent.mkdir(parents=True, exist_ok=True)
        gmsh.option.setNumber("Mesh.MshFileVersion", float(config.msh_version))
        gmsh.option.setNumber("Mesh.Binary", int(config.binary))
        gmsh.write(str(path))
        if output.write_vtk:
            gmsh.write(str(path.with_suffix(".vtk")))
        return path
