from trimuse.input.geometry_mesh.config import GeometryMeshCard
IdealMultilayerSphereConfig = GeometryMeshCard.Geometry.IdealMultilayerSphere
SurfaceZonesConfig = GeometryMeshCard.Geometry.SurfaceZones
from trimuse.geometry.topology import GeometryTopology
from trimuse.geometry.surface_zones import partition_surface_3d
import gmsh
from math import cos, pi, sin

class IdealMultilayerSphereBuilder:
    """根据空间维数构造径向线、同心圆或同心球"""

    @classmethod
    def build(cls, config: IdealMultilayerSphereConfig, dimension: int, surface_zones: SurfaceZonesConfig) -> GeometryTopology:
        """构造已通过输入检查的理想颗粒, Gmsh 生命周期由外部管理"""
        if dimension == 1:
            return cls._build_radial_line(config, surface_zones)
        if dimension == 2:
            return cls._build_axisymmetric_section(config, surface_zones)
        if dimension == 3:
            return cls._build_concentric_spheres(config, surface_zones)

    @staticmethod
    def _build_radial_line(config: IdealMultilayerSphereConfig, surface_zones: SurfaceZonesConfig) -> GeometryTopology:
        """1D 分层径向线"""

        # 准备数据
        center_x = config.center.x
        layer_names = [layer.name for layer in  config.layers.layer]
        outer_radii = [layer.outer_radius for layer in  config.layers.layer]
        boundary_names = [layer.outer_boundary.name for layer in  config.layers.layer]
        boundary_names[-1] = surface_zones.residual_zone

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
    def _build_axisymmetric_section(config: IdealMultilayerSphereConfig, surface_zones: SurfaceZonesConfig) -> GeometryTopology:
        """2D 轴对称理想多层颗粒右半截面"""

        center = config.center
        layers = config.layers.layer
        center_tag = gmsh.model.occ.addPoint(center.x, center.y, 0.0)
        top_points = []
        bottom_points = []
        arc_tags = []
        outer_segments = []
        for index, layer in enumerate(layers):
            radius = layer.outer_radius
            bottom = gmsh.model.occ.addPoint(center.x, center.y - radius, 0.0)
            right = gmsh.model.occ.addPoint(center.x + radius, center.y, 0.0)
            top = gmsh.model.occ.addPoint(center.x, center.y + radius, 0.0)
            bottom_points.append(bottom)
            top_points.append(top)
            if index == len(layers) - 1:
                cut_angles = sorted({0.0, pi / 2, pi, *(angle for zone in surface_zones.zone for angle in (zone.theta_min, zone.theta_max))}, reverse=True)
                angle_points = {pi: bottom, pi / 2: right, 0.0: top}
                for angle in cut_angles:
                    if angle not in angle_points:
                        angle_points[angle] = gmsh.model.occ.addPoint(center.x + radius * sin(angle), center.y + radius * cos(angle), 0.0)
                outer_segments = [(gmsh.model.occ.addCircleArc(angle_points[start], center_tag, angle_points[end]), (start + end) / 2) for start, end in zip(cut_angles, cut_angles[1:])]
                arc_tags.append([tag for tag, _ in outer_segments])
            else:
                arc_tags.append([gmsh.model.occ.addCircleArc(bottom, center_tag, right), gmsh.model.occ.addCircleArc(right, center_tag, top)])

        surface_tags = []
        axis_tags = [gmsh.model.occ.addLine(top_points[0], center_tag), gmsh.model.occ.addLine(center_tag, bottom_points[0])]
        kernel_loop = gmsh.model.occ.addCurveLoop([*arc_tags[0], *axis_tags])
        surface_tags.append(gmsh.model.occ.addPlaneSurface([kernel_loop]))
        for index in range(1, len(layers)):
            top_axis = gmsh.model.occ.addLine(top_points[index - 1], top_points[index])
            bottom_axis = gmsh.model.occ.addLine(bottom_points[index], bottom_points[index - 1])
            axis_tags.extend((top_axis, bottom_axis))
            loop = gmsh.model.occ.addCurveLoop([*arc_tags[index], -top_axis, *[-tag for tag in reversed(arc_tags[index - 1])], -bottom_axis])
            surface_tags.append(gmsh.model.occ.addPlaneSurface([loop]))

        gmsh.model.occ.synchronize()
        regions = {layer.name: [surface_tags[index]] for index, layer in enumerate(layers)}
        boundaries = {layer.outer_boundary.name: list(arc_tags[index]) for index, layer in enumerate(layers[:-1])}
        boundaries.update({zone.name: [] for zone in surface_zones.zone})
        boundaries[surface_zones.residual_zone] = []
        for tag, angle in outer_segments:
            name = next((zone.name for zone in surface_zones.zone if zone.theta_min <= angle <= zone.theta_max), surface_zones.residual_zone)
            boundaries[name].append(tag)
        boundaries["symmetry_axis"] = axis_tags
        return GeometryTopology(dimension=2, regions=regions, boundaries=boundaries)

    @staticmethod
    def _build_concentric_spheres(config: IdealMultilayerSphereConfig, surface_zones: SurfaceZonesConfig) -> GeometryTopology:
        """3D 理想同心球"""

        center = config.center
        layers = config.layers.layer

        # 建立所有同心球体
        sphere_tags = []
        for layer in layers:
            radius = layer.outer_radius
            sphere_tag = gmsh.model.occ.addSphere(0.0, 0.0, 0.0, radius)
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
                volume_tags.append(next(iter(layer_volumes)))
                previous_volumes = current_volumes

        # 同步几何
        gmsh.model.occ.synchronize()

        # 材料界面
        volume_surfaces = []
        for volume_tag in volume_tags:
            boundary_entities = gmsh.model.getBoundary([(3,volume_tag)],combined=False,oriented=False,recursive=False)
            volume_surfaces.append({tag for dimension,tag in boundary_entities if dimension == 2})
        boundaries = {}
        for index in range(len(layers) - 1):
            shared_surfaces = (volume_surfaces[index] & volume_surfaces[index + 1])
            boundaries[layers[index].outer_boundary.name] = sorted(shared_surfaces)
        # 外表面
        outer_entities = gmsh.model.getBoundary([(3, tag) for tag in volume_tags],combined=True,oriented=False,recursive=False)
        outer_surfaces = sorted(tag for dimension, tag in outer_entities if dimension == 2)
        # 先释放最外层体，切分其外表面，再用原来的内界面封闭该层。
        gmsh.model.occ.remove([(3, volume_tags[-1])], recursive=False)
        outer_boundaries = partition_surface_3d(outer_surfaces, surface_zones, layers[-1].outer_radius)
        outer_shell = gmsh.model.occ.addSurfaceLoop([tag for tags in outer_boundaries.values() for tag in tags])
        shells = [outer_shell]
        if len(layers) > 1:
            shells.append(gmsh.model.occ.addSurfaceLoop(boundaries[layers[-2].outer_boundary.name]))
        volume_tags[-1] = gmsh.model.occ.addVolume(shells)
        boundaries.update(outer_boundaries)
        # 显式传入曲面再传入体，平移时保留物理边界引用的曲面编号。
        entities = [(2, tag) for tags in boundaries.values() for tag in tags] + [(3, tag) for tag in volume_tags]
        gmsh.model.occ.translate(entities, center.x, center.y, center.z)
        gmsh.model.occ.synchronize()
        regions = {layer.name: [volume_tags[index]] for index, layer in enumerate(layers)}

        # 建立几何拓扑
        topology = GeometryTopology(dimension=3,regions=regions,boundaries=boundaries)

        return topology
