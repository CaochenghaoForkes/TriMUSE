"""理想多层同心球几何构造器"""

from trimuse.input.geometry_mesh_config import GeometryMeshCard
IdealMultilayerSphereConfig = GeometryMeshCard.Geometry.IdealMultilayerSphere
from trimuse.geometry.topology import GeometryTopology
import gmsh

class IdealMultilayerSphereBuilder:
    """根据空间维数构造径向线、同心圆或同心球"""

    @classmethod
    def build(cls, config, dimension) -> GeometryTopology:
        """构造理想同心球几何"""
        if dimension == 1:
            return cls._build_radial_line(config)
        if dimension == 2:
            return cls._build_concentric_circles(config)
        if dimension == 3:
            return cls._build_concentric_spheres(config)

    @staticmethod
    def _build_radial_line(config: IdealMultilayerSphereConfig) -> GeometryTopology:
        """1D 分层径向线"""

        # 准备数据
        center_x = config.center.x.value
        layer_names = [layer.name for layer in  config.layers.layer]
        outer_radii = [layer.outer_radius.value for layer in  config.layers.layer]
        boundary_names = [layer.outer_boundary.name for layer in  config.layers.layer]

        # 建立几何点
        point_tags = []
        center_tag = gmsh.model.occ.addPoint(center_x, 0.0, 0.0)
        point_tags.append(center_tag)
        for radius in outer_radii:
            point_tag = gmsh.model.occ.addPoint(center_x+radius,0.0,0.0)
            point_tags.append(point_tag)

        # 建立线
        line_tags = []
        for index in range(len(layer_names)):
            start_point = point_tags[index]
            end_point = point_tags[index+1]
            line_tag = gmsh.model.occ.addLine(start_point,end_point)
            line_tags.append(line_tag)

        # 同步几何
        gmsh.model.occ.synchronize()

        # 建立几何拓扑
        regions = {}
        boundaries = {}
        for index,layer_name in enumerate(layer_names):
            regions[layer_name] = [line_tags[index]]
        for index,boundary_name in enumerate(boundary_names):
            boundaries[boundary_name] = [point_tags[index+1]]
        boundaries["particle_center"] = [point_tags[0]]
        topology = GeometryTopology(dimension=1,regions=regions,boundaries=boundaries)

        return topology

    @staticmethod
    def _build_concentric_circles(config: IdealMultilayerSphereConfig) -> GeometryTopology:
        """2D 理想同心圆"""

        center = config.center
        layers = config.layers.layer

        circle_tags = []
        loop_tags = []

        for layer in layers:
            radius = layer.outer_radius.value
            # 外侧闭合曲线
            circle_tag = gmsh.model.occ.addCircle(center.x.value,center.y.value,0.0,radius)
            # 形成闭合曲线
            loop_tag = gmsh.model.occ.addCurveLoop([circle_tag])
            circle_tags.append(circle_tag)
            loop_tags.append(loop_tag)

        # 形成最内侧圆盘
        surface_tags = []
        kernel_surface = gmsh.model.occ.addPlaneSurface([loop_tags[0]])
        surface_tags.append(kernel_surface)
        # 建立外侧圆环
        for index in range(1,len(layers)):
            outer_loop = loop_tags[index]
            inner_loop = loop_tags[index-1]
            surface_tag = gmsh.model.occ.addPlaneSurface([outer_loop,inner_loop])
            surface_tags.append(surface_tag)

        # 同步几何
        gmsh.model.occ.synchronize()

        # 建立几何拓扑
        regions = {}
        boundaries = {}
        for index, layer in enumerate(layers):
            regions[layer.name] = [surface_tags[index]]
            boundaries[layer.outer_boundary.name] = [circle_tags[index]]
        topology = GeometryTopology(dimension=2,regions=regions,boundaries=boundaries)

        return topology

    @staticmethod
    def _build_concentric_spheres(config: IdealMultilayerSphereConfig) -> GeometryTopology:
        """3D 理想同心球"""

        center = config.center
        layers = config.layers.layer

        # 建立所有同心球体
        sphere_tags = []
        for layer in layers:
            radius = layer.outer_radius.value
            sphere_tag = gmsh.model.occ.addSphere(center.x.value,center.y.value,center.z.value,radius)
            sphere_tags.append(sphere_tag)

        # 将球体组成 Gmsh 实体对
        sphere_entities = [(3,sphere_tag) for sphere_tag in sphere_tags]

        # 切分并识别材料层
        if len(sphere_entities) == 1:
            volume_tags = [sphere_tags[0]]
        else:
            _, entity_map = gmsh.model.occ.fragment([sphere_entities[0]],sphere_entities[1:])
            mapped_volumes = [{tag for _, tag in mapped_entities} for mapped_entities in entity_map]
            volume_tags = []
            previous_volumes = set()
            for current_volumes in mapped_volumes:
                layer_volumes = current_volumes - previous_volumes
                if len(layer_volumes) != 1:
                    raise RuntimeError("无法唯一识别同心球材料层")
                volume_tags.append(next(iter(layer_volumes)))
                previous_volumes = current_volumes

        # 同步几何
        gmsh.model.occ.synchronize()

        # 材料区域
        regions = {layer.name: [volume_tags[index]] for index,layer in enumerate(layers)}
        # 材料界面
        volume_surfaces = []
        for volume_tag in volume_tags:
            boundary_entities = gmsh.model.getBoundary([(3,volume_tag)],combined=False,oriented=False,recursive=False)
            volume_surfaces.append({tag for dimension,tag in boundary_entities if dimension == 2})
        boundaries = {}
        for index in range(len(layers) - 1):
            shared_surfaces = (volume_surfaces[index] & volume_surfaces[index + 1])
            if not shared_surfaces:
                raise RuntimeError(f"未找到 {layers[index].name} 的外界面")
            boundaries[layers[index].outer_boundary.name] = sorted(shared_surfaces)
        # 外表面
        outer_entities = gmsh.model.getBoundary([(3, tag) for tag in volume_tags],combined=True,oriented=False,recursive=False)
        outer_surfaces = sorted(tag for dimension, tag in outer_entities if dimension == 2)
        boundaries[layers[-1].outer_boundary.name] = outer_surfaces

        # 建立几何拓扑
        topology = GeometryTopology(dimension=3,regions=regions,boundaries=boundaries)

        return topology
