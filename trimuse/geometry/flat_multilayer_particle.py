from trimuse.input.geometry_mesh.config import GeometryMeshCard
FlatMultilayerParticleConfig = GeometryMeshCard.Geometry.FlatMultilayerParticle
ZonesConfig = GeometryMeshCard.Geometry.Zones
from trimuse.geometry.topology import GeometryTopology
from trimuse.geometry.zones import partition_surface_3d
from dataclasses import replace
import gmsh
from math import atan2, cos, pi, sin, sqrt


class FlatMultilayerParticleBuilder:
    """截平多层颗粒: 由底部直线、相切圆角和原球圆弧构造截面及旋转体"""

    @classmethod
    def build(cls, config: FlatMultilayerParticleConfig, dimension: int, zones: ZonesConfig) -> GeometryTopology:
        """按维数生成轴对称截面或三维旋转体, Gmsh 生命周期由外部管理"""
        if dimension == 2:
            return cls._build_2d(config, zones)
        if dimension == 3:
            return cls._build_3d(config, zones)

    @classmethod
    def _build_2d(cls, config: FlatMultilayerParticleConfig, zones: ZonesConfig) -> GeometryTopology:
        """构造已通过输入检查的几何, 公共界面复用曲线, Gmsh 生命周期由外部管理"""
        center = config.center
        center_tag = gmsh.model.occ.addPoint(center.x, center.y, 0.0) # 构造圆心
        region_configs = config.regions.region
        top_points, bottom_points, contours = [], [], [] # 记录 顶点、底点、整条轮廓的曲线编号
        outer_segments = [] # 单独留给最外表面的分区
        for index, region in enumerate(region_configs): # 每次处理一层
            cut_angles = [angle for zone in zones.zone for angle in (zone.theta_min, zone.theta_max)] if index == len(region_configs) - 1 else [] # 只在最外层才需要按照 zones 的角度端点切开轮廓 例如某外边界分区是 30度~60度 这两个角度就会加入到 cut_angles 内部材料不会被切割
            bottom, top, segments = cls._build_contour(config, region.outer_radius, center_tag, cut_angles)
            bottom_points.append(bottom) # 保存该层外轮廓
            top_points.append(top) # 保存该层外轮廓
            contours.append([tag for tag, _ in segments]) # 保存该层外轮廓 服务于构造材料区域和界面
            if index == len(region_configs) - 1:
                outer_segments = segments # outer_segments 服务于给最外表面命名分区

        # 各层轮廓从底部沿右侧走向顶部, 轴线段闭合核心和各包覆层
        axis_tags = [gmsh.model.occ.addLine(top_points[0], center_tag), gmsh.model.occ.addLine(center_tag, bottom_points[0])] # kernel 的顶部 -> 球心 -> 底部
        loop = gmsh.model.occ.addCurveLoop([*contours[0], *axis_tags]) # contours[0] 是从底部到顶部的 kernel 的外轮廓线 这里连成曲线环
        surfaces = [gmsh.model.occ.addPlaneSurface([loop])] # 填出 kernel 的二维面
        for index in range(1, len(region_configs)):
            top_axis = gmsh.model.occ.addLine(top_points[index - 1], top_points[index]) # 上一层顶点 -> 当前层顶点
            bottom_axis = gmsh.model.occ.addLine(bottom_points[index], bottom_points[index - 1]) # 当前层底点 -> 上一层底点
            axis_tags.extend((top_axis, bottom_axis)) # 这两段也注册为对称轴边界
            loop = gmsh.model.occ.addCurveLoop([*contours[index], -top_axis, *[-tag for tag in reversed(contours[index - 1])], -bottom_axis]) # 沿当前层外轮廓，到当前层顶点 -> 沿 -top_axis，到上层顶点 -> 沿上一层轮廓反向走，到上一层底点 -> 沿 -bottom_axis 回到当前层底点 负编号表示把已有曲线反向使用，不会创建另一条曲线
            surfaces.append(gmsh.model.occ.addPlaneSurface([loop])) # 最后用这个闭合环填出当前材料面，并将面编号加入 surfaces

        gmsh.model.occ.synchronize() # 同步几何
        regions = {region.name: [surfaces[index]] for index, region in enumerate(region_configs)} # 把材料名称映射到对应的二维面编号
        boundaries = {region.outer_boundary.name: contours[index] for index, region in enumerate(region_configs[:-1])} # 登记内部材料界面
        boundaries.update({zone.name: [] for zone in zones.zone}) # 为每个指定的外表面 zone 和剩余区 residual_zone 建立空列表
        boundaries[zones.residual_zone] = []
        for tag, angle in outer_segments:
            name = next((zone.name for zone in zones.zone if zone.theta_min <= angle <= zone.theta_max), zones.residual_zone) # next(...) 会找第一个包含该角度的 zone，返回其名称；如果找不到，就返回默认值 zones.residual_zone
            boundaries[name].append(tag) # 把该曲线编号放进对应列表
        boundaries["symmetry_axis"] = axis_tags # 把先前收集的所有轴线段归为 symmetry_axis 边界
        return GeometryTopology(dimension=2, regions=regions, boundaries=boundaries)

    @classmethod
    def _build_3d(cls, config: FlatMultilayerParticleConfig, zones: ZonesConfig) -> GeometryTopology: 
        """旋转各层轮廓形成嵌套实心体, 切分材料层并保留共享界面"""
        occ = gmsh.model.occ
        # 先在原点构造, 外表面分区完成后统一平移到用户给定的中心
        local_config = replace(config, center=GeometryMeshCard.Geometry.Center()) # replace 复制一份配置，但将几何中心临时设为原点
        center_tag = occ.addPoint(0.0, 0.0, 0.0) # 原点编号
        solids, swept_surfaces = [], [] # solids：每层旋转所得的实心体，即“从中心填满到这一层外表面”的体 swept_surfaces：旋转时创建的曲面编号，供后面清理未被实体采用的曲面
        for region in config.regions.region: # 从内向外处理
            _, _, segments = cls._build_contour(local_config, region.outer_radius, center_tag, []) # 生成"底部直线＋圆角＋球面圆弧”的二维母线(最后传入的[]意思就是不按照二维的极角切母线,三维的外表面分区后面会统一处理)
            swept = occ.revolve([(1, tag) for tag, _ in segments], 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 2 * pi) # [(1, tag) for tag, _ in segments] 将每条母线曲线写成(维数,几何编号) 随后的三个 0.0 0.0 0.0 表示转轴经过原点 接着的三个 0.0 1.0 0.0 表示转轴方向为 +y 最后的 2\pi 表示旋转一整圈
            surfaces = [tag for dimension, tag in swept if dimension == 2] # 只提取旋转出来的 2 维曲面
            solids.append((3, occ.addVolume([occ.addSurfaceLoop(surfaces)]))) # occ.addSurfaceLoop(surfaces) 得到封闭曲面 -> occ.addVolume([]) 得到三维实心体 -> solids 记录(3,体编号)
            swept_surfaces.extend(surfaces) # 把本层旋转得到的曲面编号逐个加入总列表 OpenCASCADE 在封闭曲面、建立体时可能重新生成部分曲面，后面会对照实体实际使用的曲面，清理多余的原始曲面
        occ.synchronize()
        # 封闭旋转面时 OCC 可能重建曲面; 删除未被体采用的原始曲面
        used_surfaces = {tag for _, tag in gmsh.model.getBoundary(solids, combined=False, oriented=False)} # 查询每个实体实际使用的边界曲面 把其编号放入 used_surfaces
        occ.remove([(2, tag) for tag in swept_surfaces if tag not in used_surfaces], recursive=True) # swept_surfaces 中记录着 revolve 最初生成的曲面 但 addSurfaceLoop、addVolume 在封闭几何时，OpenCASCADE 可能重新建立某些曲面 那些原始曲面若不在 used_surfaces 中，就没有被实心体采用，可以删除
        occ.rotate(solids, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, pi / 2) # 上面得到的球体是以 +y 为对称轴的,但我们的三维几何应该以 +z 为对称轴 这里的旋转 +y -> +z -y -> -z

        if len(solids) == 1: # 若只有一层 单层实心体本身就是所需的材料体，无须切分
            volume_tags = [solids[0][1]]
        else:
            _, entity_map = occ.fragment([solids[0]], solids[1:]) # 让所有嵌套体相互切分，形成共享几何界面的独立碎片 entity_map 记录每个原实心体切分后，对应哪些新实体
            volume_tags, previous = [], set()
            for entities in entity_map:
                current = {tag for dimension, tag in entities if dimension == 3} # current : kernel 原实心体  → {kernel 体} buffer 原实心体  → {kernel 体, buffer 体} IPyC 原实心体 → {kernel 体, buffer 体, IPyC 体}
                volume_tags.append(next(iter(current - previous))) # 去掉上一层已经包含的体，留下这一层新增的材料体
                previous = current
        occ.synchronize()
        volume_surfaces = [{tag for _, tag in gmsh.model.getBoundary([(3, volume)], oriented=False)} for volume in volume_tags] # 查询每个材料体的所有边界曲面编号，分别存成一个集合
        boundaries = {region.outer_boundary.name: sorted(volume_surfaces[index] & volume_surfaces[index + 1]) for index, region in enumerate(config.regions.region[:-1])} # 找相邻两层的共同曲面(最外层暂时不处理)

        # 复用三维角域切割: 球冠/环带可跨越球面、圆角及底面, 只切分最外表面
        outer_surfaces = [tag for _, tag in gmsh.model.getBoundary([(3, tag) for tag in volume_tags], oriented=False)] # 把所有材料体作为一组查询边界 getBoundary() 默认合并查询结果：相邻材料体之间的内部共享面被抵消，留下整个颗粒的外表面
        occ.remove([(3, volume_tags[-1])], recursive=False) # 移除最外层的体，因为它的外表面即将被切分，原体的封闭曲面壳随后需要重建
        outer_boundaries = partition_surface_3d(outer_surfaces, zones, config.regions.region[-1].outer_radius) # 按 zones 指定的三维方向和角距离，把外表面切为球冠、环带及剩余区 {"heating_cap": [曲面编号1, 曲面编号2], "environment": [曲面编号3, ...],...}
        shells = [occ.addSurfaceLoop([tag for tags in outer_boundaries.values() for tag in tags])] # 把所有外表面分区中的曲面编号重新汇总，组成最外层的外侧封闭曲面壳
        if len(volume_tags) > 1: # 如果有多层，最外层材料还有一个内侧曲面壳 单层颗粒没有这个
            shells.append(occ.addSurfaceLoop(boundaries[config.regions.region[-2].outer_boundary.name]))
        volume_tags[-1] = occ.addVolume(shells) # 利用外壳，以及多层时的内壳，重新建立最外层材料体
        boundaries.update(outer_boundaries) # 把已切分的外表面分区加入边界名称字典 此时 boundaries 同时包含内部材料界面和外表面各分区

        # 曲面放在体之前, 平移时保留各层界面及分区的曲面编号
        entities = [(2, tag) for tags in boundaries.values() for tag in tags] + [(3, tag) for tag in volume_tags] # 汇总要平移的曲面与材料体
        occ.translate(entities, config.center.x, config.center.y, config.center.z) # 这里将曲面和体一起平移到输入卡指定的中心
        occ.synchronize()
        # 旋转后未被材料体采用的母线和辅助圆心不参与三维网格划分。
        for dimension in (1, 0): # 清理未被使用的线和点 1 是曲线，0 是点 曲线删掉后，它的端点也可能变成独立点，所以要按这个顺序处理
            unused = [(dimension, tag) for _, tag in gmsh.model.getEntities(dimension) if len(gmsh.model.getAdjacencies(dimension, tag)[0]) == 0] # 对曲线，查看是否有曲面使用它 对点，查看是否有曲线使用它
            occ.remove(unused)
            gmsh.model.removeEntities(unused)
            occ.synchronize()
        regions = {region.name: [volume_tags[index]] for index, region in enumerate(config.regions.region)}
        return GeometryTopology(dimension=3, regions=regions, boundaries=boundaries)

    @staticmethod
    def _build_contour(config: FlatMultilayerParticleConfig, radius: float, center_tag: int, cut_angles: list[float]) -> tuple[int, int, list[tuple[int, float]]]: # 为某一层生成从底部轴线点到顶部轴线点的右侧轮廓
        """按极角切分精确轮廓, 返回底点、顶点及 (曲线编号, 中间极角)"""
        depth, fillet = config.flattening_depth, config.fillet_radius # depth 截平深度 fillet 圆角半径
        distance = radius - depth # 球心到底部平面的距离
        # 以几何中心为原点: 底面 y=-distance, 圆角圆心到原点的距离为 R-rho
        fillet_x = sqrt(depth * (2 * (radius - fillet) - depth)) # 由几何关系导出 圆角中心 x 坐标
        fillet_y = fillet - distance # 由几何关系导出 圆角中心 y 坐标
        circle_angle = atan2(fillet_x, fillet_y)  # 原球圆弧与圆角相切处的极角 (y正方向与球心到该点射线间的夹角)
        flat_angle = atan2(fillet_x, -distance)  # 圆角与底部直线相切处的极角 (y正方向与球心到该点射线间的夹角)
        center = config.center
        fillet_tag = gmsh.model.occ.addPoint(center.x + fillet_x, center.y + fillet_y, 0.0) # 加上用户指定的球心坐标偏移 创建圆角的坐标

        # 确定轮廓需要在哪些角度断开
        angles = []
        for angle in sorted([0.0, pi / 2, circle_angle, flat_angle, pi, *cut_angles], reverse=True): # 列表包含顶部、右侧、两个曲线切点、底部，以及用户设置的外边界分区端点 然后顺序调节层从底部的 \pi 到顶部 0
            if not angles or angles[-1] - angle > 1e-12: # 相差不超过 1e-12 的弧度的角度只保留一个 避免同一位置创建一条零长度曲线
                angles.append(angle)
        points = [] # 为每个教导员算轮廓点
        for angle in angles:
            if angle == pi:
                x, y = 0.0, -distance # 底部在对称轴上的点
            elif angle <= circle_angle:
                x, y = radius * sin(angle), radius * cos(angle) # 原始球圆弧上的点
            elif angle >= flat_angle:
                x, y = -distance * sin(angle) / cos(angle), -distance # 落在底部直线上的点
            else:
                # 射线 r*(sin(theta),cos(theta)) 与圆角圆的外侧交点
                projection = fillet_x * sin(angle) + fillet_y * cos(angle) # 圆心往射线上的投影
                cross = fillet_x * cos(angle) - fillet_y * sin(angle) # 圆心指向投影点
                radial = projection + sqrt(max(0.0, fillet * fillet - cross * cross)) # 投影 + 投影到圆上目标点 P 的距离
                x, y = radial * sin(angle), radial * cos(angle) # 取得其坐标
            points.append(gmsh.model.occ.addPoint(center.x + x, center.y + y, 0.0))

        segments = []
        for index, (start, end) in enumerate(zip(angles, angles[1:])):
            angle = (start + end) / 2 # 线段中点的极角
            if angle >= flat_angle: ## 如果角度比低角还大则落在底线上
                tag = gmsh.model.occ.addLine(points[index], points[index + 1]) # 因此添加线段
            else:
                arc_center = center_tag if angle <= circle_angle else fillet_tag # 位于原球轮廓时用 center_tag 作圆心；位于过渡圆角时用 fillet_tag 作圆心
                tag = gmsh.model.occ.addCircleArc(points[index], arc_center, points[index + 1]) # 添加圆弧
            segments.append((tag, angle)) # 保存线段编号和对应的中间极角
        return points[0], points[-1], segments
