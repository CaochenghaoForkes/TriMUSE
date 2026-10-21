from math import cos, pi, sin
import gmsh
import numpy as np

from trimuse.input.geometry_mesh.config import GeometryMeshCard

def partition_surface_3d(surface_tags: list[int], zones: GeometryMeshCard.Geometry.Zones, outer_radius: float) -> dict[str, list[int]]:
    """在颗粒中心为原点的局部坐标中切分外表面，分区共用几何边"""
    residual = zones.residual_zone
    if not zones.zone:
        return {residual: surface_tags}

    occ = gmsh.model.occ
    solid = occ.addVolume([occ.addSurfaceLoop(surface_tags)]) # surface_tags: 理想或随机几何生成器得到的最外侧封闭表面
    tools = [_angular_sector(zone, outer_radius) for zone in zones.zone] # 为每个分区创建角域切割体
    _, entity_map = occ.fragment([(3, solid)], [sector for sector, _ in tools]) # 让临时实心体和所有切割体相互切分，交界处形成共用的几何面、边
    # entity_map 记录每个输入实体切分后对应哪些碎片：第 0 项对应 solid，后面的项依次对应各个切割体
    volume_groups = [{entity for entity in entities if entity[0] == 3} for entities in entity_map] # 只取三维体碎片，过滤掉其他维度
    occ.synchronize()

    # combined=True 合并原实体的所有碎片边界，消去切割体产生的内部面
    # exterior 得到的是颗粒完整外表面上所有曲面片的编号
    exterior = {tag for dimension, tag in gmsh.model.getBoundary(sorted(volume_groups[0]), combined=True, oriented=False) if dimension == 2}
    groups = {}
    for zone, tool_volumes in zip(zones.zone, volume_groups[1:]):
        intersection = volume_groups[0] & tool_volumes # 集合交集 & 找出既属于原实心体、又属于该角域切割体的体碎片 这部分就是该分区在颗粒内部对应的角域
        # 先取这些交集体的全部边界面，再与 exterior 取交集。这样就剔除了内部锥面等切割面，留下这个 zone 真正对应的颗粒外表面片
        boundary = {tag for dimension, tag in gmsh.model.getBoundary(sorted(intersection), combined=False, oriented=False) if dimension == 2}
        groups[zone.name] = exterior & boundary
    # 把所有已指定 zone 的曲面片合并，再从整个外表面中减掉，余下的就是 residual_zone
    groups[residual] = exterior - set().union(*groups.values())

    # 只保留分区后的外表面；临时体及内部切割面不进入最终材料几何
    volumes = sorted(set().union(*volume_groups))
    all_surfaces = {tag for dimension, tag in gmsh.model.getBoundary(volumes, combined=False, oriented=False) if dimension == 2}
    occ.remove(volumes, recursive=False)
    occ.remove([(2, tag) for tag in all_surfaces - exterior] + [(2, meridian) for _, meridian in tools], recursive=True)
    return {name: sorted(tags) for name, tags in groups.items()}

def _angular_sector(zone: GeometryMeshCard.Geometry.Zones.Zone, outer_radius: float) -> tuple[tuple[int, int], int]:
    """将角距离区间的圆扇形绕指定方向旋转，得到球冠或环带切割体"""
    occ = gmsh.model.occ
    direction = np.array((sin(zone.theta) * cos(zone.phi), sin(zone.theta) * sin(zone.phi), cos(zone.theta))) # 分区中心方向(转轴方向)
    tangent = np.array((cos(zone.theta) * cos(zone.phi), cos(zone.theta) * sin(zone.phi), -sin(zone.theta))) # 垂直方向
    center_tag = occ.addPoint(0.0, 0.0, 0.0) # 扇形中心点 此时几何使用以颗粒中心为原点的局部坐标
    # 以 转轴 d 垂向 t 为坐标轴 则 半径为 2R 且与转轴夹角 angle 的点落在 (2R*cos(angle),2R*sin(angle))
    # 分量再乘以 direction 与 tangent 相当于转回了笛卡尔坐标
    ends = [occ.addPoint(*(2 * outer_radius * (cos(angle) * direction + sin(angle) * tangent))) for angle in (zone.angular_distance_min, zone.angular_distance_max)]
    # 添加三条线 从中心到第一个端点的直线 以中心为圆心连接两个端点的圆弧 从第二个端点回到中心的直线
    edges = [occ.addLine(center_tag, ends[0]), occ.addCircleArc(ends[0], center_tag, ends[1]), occ.addLine(ends[1], center_tag)]
    # 形成上面三条边围成角度范围为 [alpha_min,alpha_max] 的平面扇形
    meridian = occ.addPlaneSurface([occ.addCurveLoop(edges)])
    previous_volumes = set(occ.getEntities(3)) # 记录旋转前已经存在的三维实体
    occ.revolve([(2, meridian)], 0.0, 0.0, 0.0, *direction, 2 * pi) # 将平面扇形绕经过原点的转轴转 2pi，得到球冠方向切割体或环带方向切割体
    sector, = set(occ.getEntities(3)) - previous_volumes # 用旋转后的体集合减去旋转前的体集合找到新创建的体
    return sector, meridian # 返回新体和原始扇形的编号
