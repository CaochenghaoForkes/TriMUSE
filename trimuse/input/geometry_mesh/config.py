from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from xml.etree import ElementTree as ET
import sys

from ..xml_utils import XmlUtils
from .template import GeometryMeshCardTemplate

@dataclass
class GeometryMeshCard:
    """几何与网格输入卡配置, 长度保存为 m, 无量纲参数直接保存为浮点数"""

    @dataclass
    class Model:
        """问题的空间维数"""

        dimension: int  # [1,2,3]

    @dataclass
    class Geometry:
        """几何来源及对应模型参数"""

        @dataclass
        class Center:
            """中心的笛卡尔坐标, 未输入的分量默认为零"""

            x: float = 0.0
            y: float = 0.0
            z: float = 0.0

        @dataclass
        class OuterBoundary:
            """材料层外界面的名称"""

            name: str

        @dataclass
        class SurfaceZones:
            """外表面分区角度以弧度保存；1D 只有剩余区"""

            @dataclass
            class Zone:
                name: str
                theta_min: float | None = None
                theta_max: float | None = None
                theta: float | None = None
                phi: float | None = None
                angular_distance_min: float | None = None
                angular_distance_max: float | None = None

            outer_boundary: str
            residual_zone: str
            zone: list[Zone] = field(default_factory=list)

        @dataclass
        class IdealMultilayerSphere:
            """理想多层颗粒, 按维数生成径向线, 同心圆或同心球"""

            @dataclass
            class Layers:
                """按从内到外顺序保存各层"""

                @dataclass
                class Layer:
                    """单个材料层的外半径"""

                    name: str
                    outer_boundary: GeometryMeshCard.Geometry.OuterBoundary
                    outer_radius: float

                layer: list[Layer] = field(default_factory=list)

            center: GeometryMeshCard.Geometry.Center = field(default_factory=lambda: GeometryMeshCard.Geometry.Center())
            layers: Layers = field(default_factory=Layers)

        @dataclass
        class StochasticMultilayerParticle:
            """由批次统计和外轮廓形状指标生成随机多层颗粒"""

            @dataclass
            class ShapeExpansion:
                """二维轴对称勒让德或三维球谐展开参数"""

                max_degree: int = 30
                spectrum_decay: float = 4.0

            @dataclass
            class OuterSurface:
                """外表面平均半径及旋转体球形度"""

                batch_mean_radius: float
                target_sphericity: float | None = None
                shape_expansion: GeometryMeshCard.Geometry.StochasticMultilayerParticle.ShapeExpansion | None = None

            @dataclass
            class Layers:
                """从内到外保存, 第一层使用核心半径统计, 后续层使用厚度统计"""

                @dataclass
                class Layer:
                    """单层的批次统计和颗粒内角向统计参数"""

                    name: str
                    outer_boundary: GeometryMeshCard.Geometry.OuterBoundary
                    batch_mean_radius: float | None = None
                    batch_radius_std: float | None = None
                    batch_mean_thickness: float | None = None
                    batch_thickness_std: float | None = None
                    within_particle_thickness_std: float | None = None
                    within_particle_radius_std: float | None = None
                    unwrapping_coefficient: float | None = None

                layer: list[Layer] = field(default_factory=list)

            outer_surface: OuterSurface
            center: GeometryMeshCard.Geometry.Center = field(default_factory=lambda: GeometryMeshCard.Geometry.Center())
            layers: Layers = field(default_factory=Layers)

        source: str  # [ideal_multilayer_sphere,stochastic_multilayer_particle]
        surface_zones: SurfaceZones
        ideal_multilayer_sphere: IdealMultilayerSphere | None = None
        stochastic_multilayer_particle: StochasticMultilayerParticle | None = None

    @dataclass
    class Mesh:
        """公共 Gmsh 网格配置, 单元类型由维数确定"""

        @dataclass
        class SpaceSampler:
            """随机几何角度采样, 缺省时由展开次数确定"""

            theta_interval_count: int | None = None
            phi_count: int | None = None

        @dataclass
        class Element:
            """网格几何阶次, 与后续有限元空间阶次分别设置"""

            geometry_order: int = 1  # [1,2]

        @dataclass
        class Sizing:
            """全局和区域目标网格尺寸"""

            @dataclass
            class Global:
                """全局网格尺寸上下限"""

                min_edge_length: float | None = None
                max_edge_length: float | None = None

            @dataclass
            class Region:
                """按材料层名称指定目标网格尺寸"""

                name: str
                target_edge_length: float

            global_: Global = field(default_factory=Global)  # 对应 XML global 标签
            region: list[Region] = field(default_factory=list)

        @dataclass
        class Gmsh:
            """算法与优化选项, 输出格式固定为 MSH 2.2 ASCII"""

            algorithm_3d: str = "hxt"  # [hxt,mmg3d,del3d,front3d]
            optimizer: str = "gmsh"  # [gmsh,netgen,none]

        element: Element = field(default_factory=Element)
        sizing: Sizing = field(default_factory=Sizing)
        gmsh: Gmsh = field(default_factory=Gmsh)
        space_sampler: SpaceSampler = field(default_factory=SpaceSampler)

    model: Model
    geometry: Geometry
    mesh: Mesh = field(default_factory=Mesh)

class GeometryMeshCardLoader:
    """读取几何与网格配置, 参数范围和配置一致性由 examine 检查"""

    @classmethod
    def load(cls, path: str | Path | None = None) -> GeometryMeshCard:
        """路径为空时在当前工作目录生成模板并退出, 否则读取配置"""
        if path is None or isinstance(path, str) and not path.strip():
            template_path = GeometryMeshCardTemplate.write()
            print(f"几何与网格模板输入卡已生成: {template_path}")
            sys.exit()
        root = XmlUtils.get_root(path, "geometry_mesh")
        model = cls.read_model(XmlUtils.read_child(root, "model"))
        geometry = cls.read_geometry(XmlUtils.read_child(root, "geometry"), model.dimension)
        mesh = cls.read_mesh(XmlUtils.read_child(root, "mesh", required=False))
        geometry_mesh_card = GeometryMeshCard(model=model,geometry=geometry,mesh=mesh)
        return geometry_mesh_card

    @staticmethod
    def read_model(element: ET.Element) -> GeometryMeshCard.Model:
        """读取空间维数"""
        dimension = XmlUtils.read_attribute(element, "dimension", int, required=True)
        model = GeometryMeshCard.Model(dimension=dimension)
        return model

    @classmethod
    def read_geometry(cls, element: ET.Element, dimension: int) -> GeometryMeshCard.Geometry:
        """按 source 读取对应的几何模型"""
        source = XmlUtils.read_attribute(element, "source", required=True)
        readers = {
            "ideal_multilayer_sphere": cls.read_ideal_multilayer_sphere,
            "stochastic_multilayer_particle": cls.read_stochastic_multilayer_particle
        }
        geometry_model = readers[source](XmlUtils.read_child(element, source), dimension)
        surface_zones = cls.read_surface_zones(XmlUtils.read_child(element, "surface_zones"), dimension)
        geometry = GeometryMeshCard.Geometry(source=source, surface_zones=surface_zones, **{source: geometry_model})
        return geometry

    @classmethod
    def read_surface_zones(cls, element: ET.Element, dimension: int) -> GeometryMeshCard.Geometry.SurfaceZones:
        """读取外表面命名分区，角度统一换算为弧度"""
        outer_boundary = XmlUtils.read_attribute(element, "outer_boundary", required=True)
        residual = XmlUtils.read_child(element, "residual_zone")
        residual_zone = XmlUtils.read_attribute(residual, "name", required=True)
        zones = [cls.read_surface_zone(child, dimension) for child in XmlUtils.read_children(element, "zone")]
        return GeometryMeshCard.Geometry.SurfaceZones(outer_boundary=outer_boundary, residual_zone=residual_zone, zone=zones)

    @staticmethod
    def read_surface_zone(element: ET.Element, dimension: int) -> GeometryMeshCard.Geometry.SurfaceZones.Zone:
        """按维数读取 2D 极角区间或 3D 定向球面环带"""
        name = XmlUtils.read_attribute(element, "name", required=True)
        angle_names = {2: ("theta_min", "theta_max"), 3: ("theta", "phi", "angular_distance_min", "angular_distance_max")}.get(dimension, ())
        angles = {}
        for angle_name in angle_names:
            child = XmlUtils.read_child(element, angle_name)
            angles[angle_name] = XmlUtils.read_attribute(child, "value", float, required=True, quantity="angle", context=name)
        return GeometryMeshCard.Geometry.SurfaceZones.Zone(name=name, **angles)

    @classmethod
    def read_center(cls, element: ET.Element | None) -> GeometryMeshCard.Geometry.Center:
        """读取中心坐标, 未填写分量默认为零"""
        x, y, z = 0.0, 0.0, 0.0
        if element is not None:
            x_element = XmlUtils.read_child(element, "x", required=False)
            x = XmlUtils.read_attribute(x_element, "value", float, default=0.0, quantity="length", context=element.tag)
            y_element = XmlUtils.read_child(element, "y", required=False)
            y = XmlUtils.read_attribute(y_element, "value", float, default=0.0, quantity="length", context=element.tag)
            z_element = XmlUtils.read_child(element, "z", required=False)
            z = XmlUtils.read_attribute(z_element, "value", float, default=0.0, quantity="length", context=element.tag)

        center = GeometryMeshCard.Geometry.Center(x=x, y=y, z=z)
        return center

    @staticmethod
    def read_outer_boundary(element: ET.Element) -> GeometryMeshCard.Geometry.OuterBoundary:
        """读取材料层外界面名称"""
        boundary_element = XmlUtils.read_child(element, "outer_boundary")
        name = XmlUtils.read_attribute(boundary_element, "name", required=True)
        outer_boundary = GeometryMeshCard.Geometry.OuterBoundary(name=name)
        return outer_boundary

    @classmethod
    def read_ideal_multilayer_sphere(cls, element: ET.Element, dimension: int) -> GeometryMeshCard.Geometry.IdealMultilayerSphere:
        """读取理想多层颗粒, 各层保持 XML 顺序"""
        center = cls.read_center(XmlUtils.read_child(element, "center", required=False))
        layers_element = XmlUtils.read_child(element, "layers")
        layer = [cls.read_ideal_layer(child) for child in XmlUtils.read_children(layers_element, "layer", required=True)]
        layers = GeometryMeshCard.Geometry.IdealMultilayerSphere.Layers(layer=layer)
        ideal_multilayer_sphere = GeometryMeshCard.Geometry.IdealMultilayerSphere(
            center=center,
            layers=layers
        )
        return ideal_multilayer_sphere

    @classmethod
    def read_ideal_layer(cls, element: ET.Element) -> GeometryMeshCard.Geometry.IdealMultilayerSphere.Layers.Layer:
        """读取理想材料层的外半径"""
        name = XmlUtils.read_attribute(element, "name", required=True)
        outer_boundary = cls.read_outer_boundary(element)
        outer_radius_element = XmlUtils.read_child(element, "outer_radius")
        outer_radius = XmlUtils.read_attribute(outer_radius_element, "value", float, required=True, quantity="length", context=element.tag)
        layer = GeometryMeshCard.Geometry.IdealMultilayerSphere.Layers.Layer(
            name=name,
            outer_boundary=outer_boundary,
            outer_radius=outer_radius
        )
        return layer

    @classmethod
    def read_stochastic_multilayer_particle(cls, element: ET.Element, dimension: int) -> GeometryMeshCard.Geometry.StochasticMultilayerParticle:
        """读取随机颗粒的外表面和各层统计参数"""
        outer_surface = cls.read_outer_surface(XmlUtils.read_child(element, "outer_surface"), dimension)
        center = cls.read_center(XmlUtils.read_child(element, "center", required=False))
        layers_element = XmlUtils.read_child(element, "layers")
        layer = [cls.read_stochastic_layer(child) for child in XmlUtils.read_children(layers_element, "layer", required=True)]
        layers = GeometryMeshCard.Geometry.StochasticMultilayerParticle.Layers(layer=layer)
        stochastic_multilayer_particle = GeometryMeshCard.Geometry.StochasticMultilayerParticle(
            outer_surface=outer_surface,
            center=center,
            layers=layers
        )
        return stochastic_multilayer_particle

    @classmethod
    def read_outer_surface(cls, element: ET.Element, dimension: int) -> GeometryMeshCard.Geometry.StochasticMultilayerParticle.OuterSurface:
        """读取平均外半径和形状指标, 二维三维使用默认展开参数"""
        batch_mean_radius_element = XmlUtils.read_child(element, "batch_mean_radius")
        batch_mean_radius = XmlUtils.read_attribute(batch_mean_radius_element, "value", float, required=True, quantity="length", context=element.tag)
        target_sphericity_element = XmlUtils.read_child(element, "target_sphericity", required=False)
        target_sphericity = XmlUtils.read_attribute(target_sphericity_element, "value", float, default=None, quantity="dimensionless", context=element.tag)
        expansion_element = XmlUtils.read_child(element, "shape_expansion", required=False)
        shape_expansion = cls.read_shape_expansion(expansion_element) if expansion_element is not None or dimension > 1 else None
        outer_surface = GeometryMeshCard.Geometry.StochasticMultilayerParticle.OuterSurface(
            batch_mean_radius=batch_mean_radius,
            target_sphericity=target_sphericity,
            shape_expansion=shape_expansion
        )
        return outer_surface

    @staticmethod
    def read_shape_expansion(element: ET.Element | None) -> GeometryMeshCard.Geometry.StochasticMultilayerParticle.ShapeExpansion:
        """读取频谱参数, 缺省值为次数 30 和衰减指数 4"""
        max_degree = XmlUtils.read_attribute(element, "max_degree", int, default=30)
        spectrum_decay = XmlUtils.read_attribute(element, "spectrum_decay", float, default=4.0)
        shape_expansion = GeometryMeshCard.Geometry.StochasticMultilayerParticle.ShapeExpansion(
            max_degree=max_degree,
            spectrum_decay=spectrum_decay
        )
        return shape_expansion

    @classmethod
    def read_stochastic_layer(cls, element: ET.Element) -> GeometryMeshCard.Geometry.StochasticMultilayerParticle.Layers.Layer:
        """读取存在的统计量, 核心与包覆层所需参数由 examine 检查"""
        name = XmlUtils.read_attribute(element, "name", required=True)
        outer_boundary = cls.read_outer_boundary(element)
        batch_mean_radius_element = XmlUtils.read_child(element, "batch_mean_radius", required=False)
        batch_mean_radius = XmlUtils.read_attribute(batch_mean_radius_element, "value", float, default=None, quantity="length", context=element.tag)
        batch_radius_std_element = XmlUtils.read_child(element, "batch_radius_std", required=False)
        batch_radius_std = XmlUtils.read_attribute(batch_radius_std_element, "value", float, default=None, quantity="length", context=element.tag)
        batch_mean_thickness_element = XmlUtils.read_child(element, "batch_mean_thickness", required=False)
        batch_mean_thickness = XmlUtils.read_attribute(batch_mean_thickness_element, "value", float, default=None, quantity="length", context=element.tag)
        batch_thickness_std_element = XmlUtils.read_child(element, "batch_thickness_std", required=False)
        batch_thickness_std = XmlUtils.read_attribute(batch_thickness_std_element, "value", float, default=None, quantity="length", context=element.tag)
        within_particle_thickness_std_element = XmlUtils.read_child(element, "within_particle_thickness_std", required=False)
        within_particle_thickness_std = XmlUtils.read_attribute(within_particle_thickness_std_element, "value", float, default=None, quantity="length", context=element.tag)
        within_particle_radius_std_element = XmlUtils.read_child(element, "within_particle_radius_std", required=False)
        within_particle_radius_std = XmlUtils.read_attribute(within_particle_radius_std_element, "value", float, default=None, quantity="length", context=element.tag)
        unwrapping_coefficient_element = XmlUtils.read_child(element, "unwrapping_coefficient", required=False)
        unwrapping_coefficient = XmlUtils.read_attribute(unwrapping_coefficient_element, "value", float, default=None, quantity="dimensionless", context=element.tag)
        layer = GeometryMeshCard.Geometry.StochasticMultilayerParticle.Layers.Layer(
            name=name,
            outer_boundary=outer_boundary,
            batch_mean_radius=batch_mean_radius,
            batch_radius_std=batch_radius_std,
            batch_mean_thickness=batch_mean_thickness,
            batch_thickness_std=batch_thickness_std,
            within_particle_thickness_std=within_particle_thickness_std,
            within_particle_radius_std=within_particle_radius_std,
            unwrapping_coefficient=unwrapping_coefficient
        )
        return layer

    @classmethod
    def read_mesh(cls, element: ET.Element | None) -> GeometryMeshCard.Mesh:
        """读取公共网格选项, 未提供的部分使用配置类默认值"""
        defaults = GeometryMeshCard.Mesh()
        space_sampler = defaults.space_sampler
        mesh_element, sizing, gmsh = defaults.element, defaults.sizing, defaults.gmsh
        if element is not None:
            child = XmlUtils.read_child(element, "space_sampler", required=False)
            space_sampler = GeometryMeshCard.Mesh.SpaceSampler(
                theta_interval_count=XmlUtils.read_attribute(child, "theta_interval_count", int),
                phi_count=XmlUtils.read_attribute(child, "phi_count", int)
            )
            child = XmlUtils.read_child(element, "element", required=False)
            if child is not None:
                geometry_order = XmlUtils.read_attribute(child, "geometry_order", int, default=1)
                mesh_element = GeometryMeshCard.Mesh.Element(geometry_order=geometry_order)
            child = XmlUtils.read_child(element, "sizing", required=False)
            if child is not None:
                sizing = cls.read_sizing(child)
            child = XmlUtils.read_child(element, "gmsh", required=False)
            if child is not None:
                gmsh = cls.read_gmsh(child)
        mesh = GeometryMeshCard.Mesh(
            space_sampler=space_sampler,
            element=mesh_element,
            sizing=sizing,
            gmsh=gmsh
        )
        return mesh

    @classmethod
    def read_sizing(cls, element: ET.Element) -> GeometryMeshCard.Mesh.Sizing:
        """读取全局尺寸范围和各区域目标尺寸"""
        global_sizing = GeometryMeshCard.Mesh.Sizing.Global()
        global_element = XmlUtils.read_child(element, "global", required=False)
        if global_element is not None:
            min_edge_length_element = XmlUtils.read_child(global_element, "min_edge_length", required=False)
            min_edge_length = XmlUtils.read_attribute(min_edge_length_element, "value", float, default=None, quantity="length", context=global_element.tag)
            max_edge_length_element = XmlUtils.read_child(global_element, "max_edge_length", required=False)
            max_edge_length = XmlUtils.read_attribute(max_edge_length_element, "value", float, default=None, quantity="length", context=global_element.tag)
            global_sizing = GeometryMeshCard.Mesh.Sizing.Global(
                min_edge_length=min_edge_length,
                max_edge_length=max_edge_length
            )
        regions = []
        for region_element in XmlUtils.read_children(element, "region"):
            name = XmlUtils.read_attribute(region_element, "name", required=True)
            target_edge_length_element = XmlUtils.read_child(region_element, "target_edge_length")
            target_edge_length = XmlUtils.read_attribute(target_edge_length_element, "value", float, required=True, quantity="length", context=region_element.tag)
            region = GeometryMeshCard.Mesh.Sizing.Region(name=name, target_edge_length=target_edge_length)
            regions.append(region)
        sizing = GeometryMeshCard.Mesh.Sizing(global_=global_sizing, region=regions)
        return sizing

    @staticmethod
    def read_gmsh(element: ET.Element) -> GeometryMeshCard.Mesh.Gmsh:
        """读取网格算法和优化选项"""
        algorithm_3d = XmlUtils.read_attribute(element, "algorithm_3d", default="hxt")
        optimizer = XmlUtils.read_attribute(element, "optimizer", default="gmsh")
        gmsh = GeometryMeshCard.Mesh.Gmsh(algorithm_3d=algorithm_3d, optimizer=optimizer)
        return gmsh
