from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from xml.etree import ElementTree as ET

from .units import Units
from .xml_utils import XmlUtils

@dataclass
class UnitValue:
    """带物理单位的标量数值"""

    value: float
    unit: str

@dataclass
class GeometryMeshCard:
    """几何与网格生成任务的完整输入卡"""

    @dataclass
    class Model:
        """待建模问题的空间维数"""

        dimension: int  # [1,2,3]

    @dataclass
    class Geometry:
        """颗粒几何的定义方式与几何参数"""

        @dataclass
        class IdealMultilayerSphere:
            """理想多层同心球模型的几何参数"""

            @dataclass
            class Center:
                """球心的空间坐标"""

                x: UnitValue = field(default_factory=lambda: UnitValue(0.0, "[m]"))
                y: UnitValue = field(default_factory=lambda: UnitValue(0.0, "[m]"))
                z: UnitValue = field(default_factory=lambda: UnitValue(0.0, "[m]"))

            @dataclass
            class Layers:
                """从内到外排列的同心层序列"""

                @dataclass
                class Layer:
                    """单个同心材料层"""

                    @dataclass
                    class OuterBoundary:
                        """该层外表面对应的几何接口名称"""

                        name: str

                    name: str
                    outer_boundary: OuterBoundary
                    outer_radius: UnitValue  # 该层外表面的半径

                layer: list[Layer] = field(default_factory=list)

            center: Center = field(default_factory=Center)
            layers: Layers = field(default_factory=Layers)

        @dataclass
        class StochasticMultilayerParticle:
            """基于批次统计的随机多层颗粒"""

            @dataclass
            class ShapeExpansion:
                """二维傅里叶或三维球谐展开"""

                max_degree: int = 30
                spectrum_decay: float = 4.0  # 暂定模型参数, 后续标定

            @dataclass
            class OuterSurface:
                """外表面尺寸与形状指标"""

                batch_mean_radius: UnitValue
                target_circularity: UnitValue | None = None
                target_sphericity: UnitValue | None = None
                shape_expansion: GeometryMeshCard.Geometry.StochasticMultilayerParticle.ShapeExpansion | None = None

            @dataclass
            class Layers:
                """从内到外排列, 第一层为核心"""

                @dataclass
                class Layer:
                    """核心半径或包覆层厚度的统计输入"""

                    name: str
                    outer_boundary: GeometryMeshCard.Geometry.IdealMultilayerSphere.Layers.Layer.OuterBoundary
                    batch_mean_radius: UnitValue | None = None
                    batch_radius_std: UnitValue | None = None
                    batch_mean_thickness: UnitValue | None = None
                    batch_thickness_std: UnitValue | None = None
                    within_particle_thickness_std: UnitValue | None = None
                    within_particle_radius_std: UnitValue | None = None
                    unwrapping_coefficient: UnitValue | None = None

                layer: list[Layer] = field(default_factory=list)

            outer_surface: OuterSurface
            center: GeometryMeshCard.Geometry.IdealMultilayerSphere.Center = field(default_factory=lambda: GeometryMeshCard.Geometry.IdealMultilayerSphere.Center())
            layers: Layers = field(default_factory=Layers)

        source: str  # [ideal_multilayer_sphere,stochastic_multilayer_particle]
        ideal_multilayer_sphere: IdealMultilayerSphere | None = None
        stochastic_multilayer_particle: StochasticMultilayerParticle | None = None

    @dataclass
    class Mesh:
        """网格生成的引擎选择与全部控制参数"""

        @dataclass
        class Element:
            """网格单元的类型与几何阶次"""

            type: str = "tetrahedron"  # [line,triangle,quadrilateral,tetrahedron,hexahedron,prism,pyramid]
            geometry_order: int = 1  # [1,2]

        @dataclass
        class Sizing:
            """网格剖分的全局与区域尺寸控制"""

            @dataclass
            class Global:
                """全域网格边长的允许范围"""

                min_edge_length: UnitValue | None = None  # 网格单元边长的全局下限
                max_edge_length: UnitValue | None = None  # 网格单元边长的全局上限

            @dataclass
            class Region:
                """指定几何区域的目标网格尺寸"""

                name: str
                target_edge_length: UnitValue  # 该区域内网格的目标边长

            # 字段对应 XML 标签 <global>，因与 Python 关键字重名加下划线
            global_: Global = field(default_factory=Global)
            region: list[Region] = field(default_factory=list)

        @dataclass
        class Output:
            """网格文件的写出路径"""

            file: str = "./output/mesh/mesh.msh"
            write_vtk: bool = False

        @dataclass
        class Gmsh:
            """Gmsh 网格引擎的算法与输出格式选项"""

            algorithm_3d: str = "hxt"  # [hxt,mmg3d,del3d,front3d]
            optimizer: str = "gmsh"  # [gmsh,netgen,none]
            msh_version: str = "4.1"  # [2.2,4.1]
            binary: bool = False

        engine: str  # [gmsh]
        element: Element = field(default_factory=Element)
        sizing: Sizing = field(default_factory=Sizing)
        output: Output = field(default_factory=Output)
        gmsh: Gmsh = field(default_factory=Gmsh)

    schema_version: str  # [0.1]
    model: Model
    geometry: Geometry
    mesh: Mesh

class GeometryMeshCardTemplate:
    """维护完整节点模板并提供文件写出入口"""

    CONTENT = '''<?xml version="1.0" encoding="UTF-8"?>
<!-- 填写占位符后使用, 枚举值只选一项, 不使用的可选节点请删除 -->
<!-- 长度 unit 填写合法单位字符串, 如 [um] 或 [m], 无量纲填写 [1] -->
<geometry_mesh schema_version="[0.1]">
    <model dimension="[1][2][3]"/>
    <!-- 根据 source 只保留对应的几何子节点 -->
    <geometry source="[ideal_multilayer_sphere][stochastic_multilayer_particle]">
        <ideal_multilayer_sphere>
            <!-- center 可省略, 坐标缺省为零, 1D 只保留 x, 2D 保留 x 和 y -->
            <center>
                <x value="[float]" unit="[str]"/>
                <y value="[float]" unit="[str]"/>
                <z value="[float]" unit="[str]"/>
            </center>
            <layers>
                <!-- 按从内到外顺序重复 layer, outer_radius 必须严格递增 -->
                <layer name="[str]">
                    <outer_boundary name="[str]"/>
                    <outer_radius value="[float]" unit="[str]"/>
                </layer>
            </layers>
        </ideal_multilayer_sphere>
        <!-- 输入读取已支持, 随机几何构造器尚未实现 -->
        <stochastic_multilayer_particle>
            <!-- center 的规则同理想几何 -->
            <center>
                <x value="[float]" unit="[str]"/>
                <y value="[float]" unit="[str]"/>
                <z value="[float]" unit="[str]"/>
            </center>
            <outer_surface>
                <batch_mean_radius value="[float]" unit="[str]"/>
                <!-- 1D 删除以下三个节点, 2D 保留圆形度, 3D 保留球形度 -->
                <target_circularity value="[float]" unit="[str]"/>
                <target_sphericity value="[float]" unit="[str]"/>
                <!-- 可省略, max_degree 默认 30, spectrum_decay 暂定默认 4 -->
                <shape_expansion max_degree="[int]" spectrum_decay="[float]"/>
            </outer_surface>
            <layers>
                <!-- 第一层为核心, 名称自定, 使用半径的批次统计 -->
                <layer name="[str]">
                    <outer_boundary name="[str]"/>
                    <batch_mean_radius value="[float]" unit="[str]"/>
                    <batch_radius_std value="[float]" unit="[str]"/>
                </layer>
                <!-- 后续包覆层按从内到外顺序重复, 单层颗粒删除此项 -->
                <layer name="[str]">
                    <outer_boundary name="[str]"/>
                    <batch_mean_thickness value="[float]" unit="[str]"/>
                    <batch_thickness_std value="[float]" unit="[str]"/>
                    <!-- 1D 删除以下三项, 2D 和 3D 必填 -->
                    <within_particle_thickness_std value="[float]" unit="[str]"/>
                    <within_particle_radius_std value="[float]" unit="[str]"/>
                    <unwrapping_coefficient value="[float]" unit="[str]"/>
                </layer>
            </layers>
        </stochastic_multilayer_particle>
    </geometry>
    <mesh engine="[gmsh]">
        <!-- 1D 选 line, 2D 选面单元, 3D 选体单元 -->
        <!-- 当前构造器仅支持 line, triangle, tetrahedron, 其他类型预留 -->
        <element type="[line][triangle][quadrilateral][tetrahedron][hexahedron][prism][pyramid]" geometry_order="[1][2]"/>
        <sizing>
            <!-- 保留 global 节点, min_edge_length 和 max_edge_length 均可省略 -->
            <global>
                <min_edge_length value="[float]" unit="[str]"/>
                <max_edge_length value="[float]" unit="[str]"/>
            </global>
            <!-- region 可省略或重复, name 引用材料层名称 -->
            <region name="[str]">
                <target_edge_length value="[float]" unit="[str]"/>
            </region>
        </sizing>
        <!-- file 默认 ./output/mesh/mesh.msh, write_vtk 默认 false -->
        <output file="[str]" write_vtk="[true][false]"/>
        <!-- 属性可省略, 默认依次为 hxt, gmsh, 4.1, false -->
        <gmsh algorithm_3d="[hxt][mmg3d][del3d][front3d]" optimizer="[gmsh][netgen][none]" msh_version="[2.2][4.1]" binary="[true][false]"/>
    </mesh>
</geometry_mesh>
'''

    @classmethod
    def write(cls, path: str | Path = "geometry_mesh_empty.xml") -> Path:
        """生成待填写输入卡, 默认写入当前工作目录"""
        return XmlUtils.write_template(cls.CONTENT, path)

class GeometryMeshCardLoader:
    """几何与网格 XML 输入卡读取器"""

    @classmethod
    def load(cls, path: str | Path | None = None) -> GeometryMeshCard | Path:
        """路径为空时生成模板, 否则读取配置"""
        if path is None or isinstance(path, str) and not path.strip():
            return GeometryMeshCardTemplate.write()
        card_path = Path(path).expanduser().resolve()
        context = str(card_path)
        root = XmlUtils.read_xml(card_path, root_tag="geometry_mesh")
        XmlUtils.check_attributes(root, ("schema_version",), context=context)
        XmlUtils.check_children(root, ("model", "geometry", "mesh"), context=context)

        schema_version = XmlUtils.read_string(root,"schema_version",required=True,context=context)
        model_element = XmlUtils.read_child(root, "model", context=context)
        geometry_element = XmlUtils.read_child(root, "geometry", context=context)
        mesh_element = XmlUtils.read_child(root, "mesh", context=context)

        assert schema_version is not None
        assert model_element is not None
        assert geometry_element is not None
        assert mesh_element is not None
        model = cls.read_model(model_element, context=context)
        geometry = cls.read_geometry(
            geometry_element,
            dimension=model.dimension,
            context=context,
        )
        mesh = cls.read_mesh(mesh_element, context=context)
        return GeometryMeshCard(
            schema_version=schema_version,
            model=model,
            geometry=geometry,
            mesh=mesh,
        )

    @staticmethod
    def read_model(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> GeometryMeshCard.Model:
        """读取模型空间维数"""
        XmlUtils.check_attributes(element, ("dimension",), context=context)
        XmlUtils.check_children(element, (), context=context)
        dimension = XmlUtils.read_int(element,"dimension",required=True,context=context)
        assert dimension is not None
        return GeometryMeshCard.Model(dimension=dimension)

    @staticmethod
    def read_geometry(
        element: ET.Element,
        *,
        dimension: int,
        context: str | None = None,
    ) -> GeometryMeshCard.Geometry:
        """按 source 分发并读取对应的几何子模型"""
        XmlUtils.check_attributes(element, ("source",), context=context)
        source = XmlUtils.read_string(element, "source", required=True, context=context)
        assert source is not None

        if source == "ideal_multilayer_sphere":
            XmlUtils.check_children(element,("ideal_multilayer_sphere",),context=context)
            sphere_element = XmlUtils.read_child(element,"ideal_multilayer_sphere",context=context)
            assert sphere_element is not None
            return GeometryMeshCard.Geometry(
                source=source,
                ideal_multilayer_sphere=GeometryMeshCardLoader.read_ideal_multilayer_sphere(sphere_element,dimension=dimension,context=context),
            )

        if source == "stochastic_multilayer_particle":
            XmlUtils.check_children(element, (source,), context=context)
            child = XmlUtils.read_child(element, source, context=context)
            assert child is not None
            particle = GeometryMeshCardLoader.read_stochastic_multilayer_particle(child, dimension=dimension, context=context)
            return GeometryMeshCard.Geometry(source=source, stochastic_multilayer_particle=particle)

        raise ValueError(f"{XmlUtils._location(element, context)}: 暂不支持 geometry source {source!r}")

    @classmethod
    def read_stochastic_multilayer_particle(cls, element: ET.Element, *, dimension: int, context: str | None = None) -> GeometryMeshCard.Geometry.StochasticMultilayerParticle:
        """读取随机颗粒的种子, 中心, 外表面与各层统计"""
        if dimension not in (1, 2, 3):
            raise ValueError("随机颗粒 dimension 必须为 1, 2 或 3")
        XmlUtils.check_attributes(element, (), context=context)
        XmlUtils.check_children(element, ("center", "outer_surface", "layers"), context=context)
        center_element = XmlUtils.read_child(element, "center", required=False, context=context)
        surface_element = XmlUtils.read_child(element, "outer_surface", context=context)
        layers_element = XmlUtils.read_child(element, "layers", context=context)
        assert surface_element is not None and layers_element is not None
        center = GeometryMeshCard.Geometry.IdealMultilayerSphere.Center()
        if center_element is not None:
            center = cls.read_center(center_element, dimension=dimension, context=context)
        surface = cls.read_stochastic_outer_surface(surface_element, dimension=dimension, context=context)
        layers = cls.read_stochastic_layers(layers_element, dimension=dimension, context=context)
        return GeometryMeshCard.Geometry.StochasticMultilayerParticle(center=center, outer_surface=surface, layers=layers)

    @classmethod
    def read_stochastic_outer_surface(cls, element: ET.Element, *, dimension: int, context: str | None = None) -> GeometryMeshCard.Geometry.StochasticMultilayerParticle.OuterSurface:
        """一维只读尺寸, 二维读取圆形度, 三维读取球形度"""
        config = GeometryMeshCard.Geometry.StochasticMultilayerParticle
        shape_name = {2: "target_circularity", 3: "target_sphericity"}.get(dimension)
        allowed = ("batch_mean_radius",) if dimension == 1 else ("batch_mean_radius", shape_name, "shape_expansion")
        XmlUtils.check_attributes(element, (), context=context)
        XmlUtils.check_children(element, allowed, context=context)
        radius = XmlUtils.read_child(element, "batch_mean_radius", context=context)
        assert radius is not None
        surface = config.OuterSurface(batch_mean_radius=cls.read_length(radius, context=context))
        if shape_name is not None:
            shape = XmlUtils.read_child(element, shape_name, context=context)
            expansion = XmlUtils.read_child(element, "shape_expansion", required=False, context=context)
            assert shape is not None
            setattr(surface, shape_name, cls.read_dimensionless(shape, context=context))
            surface.shape_expansion = config.ShapeExpansion() if expansion is None else cls.read_shape_expansion(expansion, context=context)
        return surface

    @staticmethod
    def read_shape_expansion(element: ET.Element, *, context: str | None = None) -> GeometryMeshCard.Geometry.StochasticMultilayerParticle.ShapeExpansion:
        """读取展开次数和频谱衰减指数"""
        config = GeometryMeshCard.Geometry.StochasticMultilayerParticle.ShapeExpansion
        XmlUtils.check_attributes(element, ("max_degree", "spectrum_decay"), context=context)
        XmlUtils.check_children(element, (), context=context)
        degree = XmlUtils.read_int(element, "max_degree", default=config.max_degree, context=context)
        decay = XmlUtils.read_float(element, "spectrum_decay", default=config.spectrum_decay, context=context)
        assert degree is not None and decay is not None
        return config(max_degree=degree, spectrum_decay=decay)

    @classmethod
    def read_stochastic_layers(cls, element: ET.Element, *, dimension: int, context: str | None = None) -> GeometryMeshCard.Geometry.StochasticMultilayerParticle.Layers:
        """按顺序读取核心与包覆层, 不依赖材料名称"""
        XmlUtils.check_attributes(element, (), context=context)
        XmlUtils.check_children(element, ("layer",), context=context)
        layers = GeometryMeshCard.Geometry.StochasticMultilayerParticle.Layers()
        for index, child in enumerate(XmlUtils.read_children(element, "layer", required=True, context=context)):
            layers.layer.append(cls.read_stochastic_layer(child, is_core=index == 0, dimension=dimension, context=context))
        return layers

    @classmethod
    def read_stochastic_layer(cls, element: ET.Element, *, is_core: bool, dimension: int, context: str | None = None) -> GeometryMeshCard.Geometry.StochasticMultilayerParticle.Layers.Layer:
        """核心读取半径统计, 包覆层读取厚度及局部统计"""
        lengths = ("batch_mean_radius", "batch_radius_std") if is_core else ("batch_mean_thickness", "batch_thickness_std")
        local_shape = not is_core and dimension > 1
        if local_shape:
            lengths += ("within_particle_thickness_std", "within_particle_radius_std")
        allowed = ("outer_boundary",) + lengths + (("unwrapping_coefficient",) if local_shape else ())
        XmlUtils.check_attributes(element, ("name",), context=context)
        XmlUtils.check_children(element, allowed, context=context)
        name = XmlUtils.read_string(element, "name", required=True, context=context)
        boundary = XmlUtils.read_child(element, "outer_boundary", context=context)
        assert name is not None and boundary is not None
        layer = GeometryMeshCard.Geometry.StochasticMultilayerParticle.Layers.Layer(name=name, outer_boundary=cls.read_outer_boundary(boundary, context=context))
        for field_name in lengths:
            child = XmlUtils.read_child(element, field_name, context=context)
            assert child is not None
            setattr(layer, field_name, cls.read_length(child, context=context))
        if local_shape:
            child = XmlUtils.read_child(element, "unwrapping_coefficient", context=context)
            assert child is not None
            layer.unwrapping_coefficient = cls.read_dimensionless(child, context=context)
        return layer

    @staticmethod
    def read_dimensionless(element: ET.Element, *, context: str | None = None) -> UnitValue:
        """读取无量纲数值并统一单位"""
        value, unit = XmlUtils.read_quantity(element, context=context)
        return UnitValue(Units.convert(value, unit, "[1]", context=context), "[1]")

    @staticmethod
    def read_ideal_multilayer_sphere(
        element: ET.Element,
        *,
        dimension: int,
        context: str | None = None,
    ) -> GeometryMeshCard.Geometry.IdealMultilayerSphere:
        """读取理想多层同心球几何参数"""
        XmlUtils.check_attributes(element, (), context=context)
        XmlUtils.check_children(element, ("center", "layers"), context=context)
        center_element = XmlUtils.read_child(element, "center", required=False, context=context)
        layers_element = XmlUtils.read_child(element, "layers", context=context)
        center = GeometryMeshCard.Geometry.IdealMultilayerSphere.Center()
        if center_element is not None:
            center = GeometryMeshCardLoader.read_center(center_element,dimension=dimension,context=context)
        assert layers_element is not None
        layers = GeometryMeshCardLoader.read_layers(layers_element, context=context)
        return GeometryMeshCard.Geometry.IdealMultilayerSphere(center=center,layers=layers)

    @staticmethod
    def read_center(
        element: ET.Element,
        *,
        dimension: int,
        context: str | None = None,
    ) -> GeometryMeshCard.Geometry.IdealMultilayerSphere.Center:
        """读取球心坐标，未提供的分量取零"""
        coordinate_names = ("x", "y", "z")[:dimension]
        XmlUtils.check_attributes(element, (), context=context)
        XmlUtils.check_children(element, coordinate_names, context=context)
        center = GeometryMeshCard.Geometry.IdealMultilayerSphere.Center()
        for name in coordinate_names:
            coordinate_element = XmlUtils.read_child(element,name,required=False,context=context)
            if coordinate_element is not None:
                setattr(
                    center,
                    name,
                    GeometryMeshCardLoader.read_length(coordinate_element,context=context),
                )
        return center

    @staticmethod
    def read_layers(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> GeometryMeshCard.Geometry.IdealMultilayerSphere.Layers:
        """按 XML 书写顺序读取材料层"""
        XmlUtils.check_attributes(element, (), context=context)
        XmlUtils.check_children(element, ("layer",), context=context)
        layer_elements = XmlUtils.read_children(element,"layer",required=True,context=context)
        layers = GeometryMeshCard.Geometry.IdealMultilayerSphere.Layers()
        for layer_element in layer_elements:
            layers.layer.append(GeometryMeshCardLoader.read_layer(layer_element, context=context))
        return layers

    @staticmethod
    def read_layer(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> GeometryMeshCard.Geometry.IdealMultilayerSphere.Layers.Layer:
        """读取单个材料层"""
        XmlUtils.check_attributes(element, ("name",), context=context)
        XmlUtils.check_children(element,("outer_boundary", "outer_radius"),context=context)
        name = XmlUtils.read_string(element, "name", required=True, context=context)
        boundary_element = XmlUtils.read_child(element,"outer_boundary",context=context)
        radius_element = XmlUtils.read_child(element, "outer_radius", context=context)
        assert name is not None
        assert boundary_element is not None
        assert radius_element is not None
        return GeometryMeshCard.Geometry.IdealMultilayerSphere.Layers.Layer(
            name=name,
            outer_boundary=GeometryMeshCardLoader.read_outer_boundary(
                boundary_element,
                context=context,
            ),
            outer_radius=GeometryMeshCardLoader.read_length(
                radius_element,
                context=context,
            ),
        )

    @staticmethod
    def read_outer_boundary(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> GeometryMeshCard.Geometry.IdealMultilayerSphere.Layers.Layer.OuterBoundary:
        """读取材料层外边界名称"""
        XmlUtils.check_attributes(element, ("name",), context=context)
        XmlUtils.check_children(element, (), context=context)
        name = XmlUtils.read_string(element, "name", required=True, context=context)
        assert name is not None
        return GeometryMeshCard.Geometry.IdealMultilayerSphere.Layers.Layer.OuterBoundary(name=name)

    @staticmethod
    def read_mesh(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> GeometryMeshCard.Mesh:
        """读取网格生成器及网格控制参数"""
        XmlUtils.check_attributes(element, ("engine",), context=context)
        XmlUtils.check_children(element,("element", "sizing", "output", "gmsh"),context=context)
        engine = XmlUtils.read_string(element, "engine", required=True, context=context)
        element_element = XmlUtils.read_child(element, "element", context=context)
        sizing_element = XmlUtils.read_child(element, "sizing", context=context)
        output_element = XmlUtils.read_child(element, "output", context=context)
        gmsh_element = XmlUtils.read_child(element, "gmsh", context=context)
        assert engine is not None
        assert element_element is not None
        assert sizing_element is not None
        assert output_element is not None
        assert gmsh_element is not None
        return GeometryMeshCard.Mesh(
            engine=engine,
            element=GeometryMeshCardLoader.read_element(element_element, context=context),
            sizing=GeometryMeshCardLoader.read_sizing(sizing_element, context=context),
            output=GeometryMeshCardLoader.read_output(output_element, context=context),
            gmsh=GeometryMeshCardLoader.read_gmsh(gmsh_element, context=context),
        )

    @staticmethod
    def read_element(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> GeometryMeshCard.Mesh.Element:
        """读取网格单元类型和几何阶次"""
        XmlUtils.check_attributes(element, ("type", "geometry_order"), context=context)
        XmlUtils.check_children(element, (), context=context)
        element_type = XmlUtils.read_string(
            element,
            "type",
            default=GeometryMeshCard.Mesh.Element.type,
            context=context,
        )
        geometry_order = XmlUtils.read_int(
            element,
            "geometry_order",
            default=GeometryMeshCard.Mesh.Element.geometry_order,
            context=context,
        )
        assert element_type is not None
        assert geometry_order is not None
        return GeometryMeshCard.Mesh.Element(
            type=element_type,
            geometry_order=geometry_order,
        )

    @staticmethod
    def read_sizing(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> GeometryMeshCard.Mesh.Sizing:
        """读取全局和分区网格尺寸"""
        XmlUtils.check_attributes(element, (), context=context)
        XmlUtils.check_children(element, ("global", "region"), context=context)
        global_element = XmlUtils.read_child(element, "global", context=context)
        region_elements = XmlUtils.read_children(element, "region", context=context)
        assert global_element is not None
        sizing = GeometryMeshCard.Mesh.Sizing(
            global_=GeometryMeshCardLoader.read_global(global_element, context=context)
        )
        for region_element in region_elements:
            sizing.region.append(
                GeometryMeshCardLoader.read_region(region_element, context=context)
            )
        return sizing

    @staticmethod
    def read_global(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> GeometryMeshCard.Mesh.Sizing.Global:
        """读取全局网格边长上下限"""
        XmlUtils.check_attributes(element, (), context=context)
        XmlUtils.check_children(
            element,
            ("min_edge_length", "max_edge_length"),
            context=context,
        )
        min_element = XmlUtils.read_child(
            element,
            "min_edge_length",
            required=False,
            context=context,
        )
        max_element = XmlUtils.read_child(
            element,
            "max_edge_length",
            required=False,
            context=context,
        )
        return GeometryMeshCard.Mesh.Sizing.Global(
            min_edge_length=(
                GeometryMeshCardLoader.read_length(min_element, context=context)
                if min_element is not None
                else None
            ),
            max_edge_length=(
                GeometryMeshCardLoader.read_length(max_element, context=context)
                if max_element is not None
                else None
            ),
        )

    @staticmethod
    def read_region(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> GeometryMeshCard.Mesh.Sizing.Region:
        """读取区域目标网格边长"""
        XmlUtils.check_attributes(element, ("name",), context=context)
        XmlUtils.check_children(element, ("target_edge_length",), context=context)
        name = XmlUtils.read_string(element, "name", required=True, context=context)
        target_element = XmlUtils.read_child(
            element,
            "target_edge_length",
            context=context,
        )
        assert name is not None
        assert target_element is not None
        return GeometryMeshCard.Mesh.Sizing.Region(
            name=name,
            target_edge_length=GeometryMeshCardLoader.read_length(
                target_element,
                context=context,
            ),
        )

    @staticmethod
    def read_output(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> GeometryMeshCard.Mesh.Output:
        """读取网格输出文件路径和可视化格式"""
        XmlUtils.check_attributes(element, ("file", "write_vtk"), context=context)
        XmlUtils.check_children(element, (), context=context)
        file = XmlUtils.read_string(
            element,
            "file",
            default=GeometryMeshCard.Mesh.Output.file,
            context=context,
        )
        write_vtk = XmlUtils.read_bool(
            element,
            "write_vtk",
            default=GeometryMeshCard.Mesh.Output.write_vtk,
            context=context,
        )
        assert file is not None
        assert write_vtk is not None
        return GeometryMeshCard.Mesh.Output(file=file, write_vtk=write_vtk)

    @staticmethod
    def read_gmsh(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> GeometryMeshCard.Mesh.Gmsh:
        """读取 Gmsh 专有参数"""
        XmlUtils.check_attributes(
            element,
            ("algorithm_3d", "optimizer", "msh_version", "binary"),
            context=context,
        )
        XmlUtils.check_children(element, (), context=context)
        algorithm_3d = XmlUtils.read_string(
            element,
            "algorithm_3d",
            default=GeometryMeshCard.Mesh.Gmsh.algorithm_3d,
            context=context,
        )
        optimizer = XmlUtils.read_string(
            element,
            "optimizer",
            default=GeometryMeshCard.Mesh.Gmsh.optimizer,
            context=context,
        )
        msh_version = XmlUtils.read_string(
            element,
            "msh_version",
            default=GeometryMeshCard.Mesh.Gmsh.msh_version,
            context=context,
        )
        binary = XmlUtils.read_bool(
            element,
            "binary",
            default=GeometryMeshCard.Mesh.Gmsh.binary,
            context=context,
        )
        assert algorithm_3d is not None
        assert optimizer is not None
        assert msh_version is not None
        assert binary is not None
        return GeometryMeshCard.Mesh.Gmsh(
            algorithm_3d=algorithm_3d,
            optimizer=optimizer,
            msh_version=msh_version,
            binary=binary,
        )

    @staticmethod
    def read_unit_value(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> UnitValue:
        """读取 value 和 unit 组成的原始物理量"""
        value, unit = XmlUtils.read_quantity(element, context=context)
        return UnitValue(value=value, unit=unit)

    @staticmethod
    def read_length(
        element: ET.Element,
        *,
        context: str | None = None,
    ) -> UnitValue:
        """读取长度并转换为内部米制单位"""
        quantity = GeometryMeshCardLoader.read_unit_value(element, context=context)
        value = Units.to_si(quantity.value, quantity.unit, "length", context=context)
        return UnitValue(value=value, unit=Units.LENGTH)

def load_input_card(path: str | Path | None = None) -> GeometryMeshCard | Path:
    """兼容旧入口的几何与网格 XML 输入卡读取函数"""
    return GeometryMeshCardLoader.load(path)

def write_geometry_mesh_template(path: str | Path = "geometry_mesh_empty.xml") -> Path:
    """生成完整 XML 模板并返回路径, 不覆盖已有文件"""
    return GeometryMeshCardTemplate.write(path)
