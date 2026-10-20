import math

from .config import GeometryMeshCard

class GeometryMeshCardExamine:
    """几何与网格配置的基本检查, 不重复读取阶段的类型和单位检查"""

    @classmethod
    def examine(cls, card: GeometryMeshCard) -> None:
        """检查维数, 几何参数和网格配置"""
        dimension = card.model.dimension
        if dimension not in (1, 2, 3):
            raise ValueError("model dimension 必须为 [1,2,3]")
        source = card.geometry.source
        if source not in ("ideal_multilayer_sphere", "stochastic_multilayer_particle"):
            raise ValueError(f"不支持的几何来源: {source}")
        geometry = getattr(card.geometry, source)
        if not all(math.isfinite(value) for value in (geometry.center.x, geometry.center.y, geometry.center.z)):
            raise ValueError("center 坐标必须有限")
        layers = geometry.layers.layer
        cls.check_layers(layers)
        if source == "ideal_multilayer_sphere":
            cls.check_ideal_layers(layers)
        else:
            cls.check_stochastic_geometry(geometry, dimension)
        cls.check_surface_zones(card.geometry.surface_zones, layers, dimension)
        cls.check_mesh(card.mesh, dimension, {layer.name for layer in layers})

    @staticmethod
    def check_surface_zones(zones: GeometryMeshCard.Geometry.SurfaceZones, layers: list, dimension: int) -> None:
        """核对外表面引用、分区名称及对应维数的角度范围"""
        if zones.outer_boundary != layers[-1].outer_boundary.name:
            raise ValueError("surface_zones.outer_boundary 必须引用最外层 outer_boundary")
        names = [zone.name for zone in zones.zone] + [zones.residual_zone]
        if len(names) != len(set(names)):
            raise ValueError("surface_zones 中的 zone 和 residual_zone 名称不能重复")
        reserved = {layer.outer_boundary.name for layer in layers} | {"particle_center", "symmetry_axis"}
        if set(names) & reserved:
            raise ValueError("外表面分区名称不能与已有几何边界名称重复")
        if dimension == 1:
            if zones.zone:
                raise ValueError("1D 外表面只能配置 residual_zone")
        elif dimension == 2:
            intervals = []
            for zone in zones.zone:
                lower, upper = zone.theta_min, zone.theta_max
                if lower is None or upper is None or not all(map(math.isfinite, (lower, upper))) or not 0 <= lower < upper <= math.pi:
                    raise ValueError(f"2D 分区 {zone.name}: theta_min/theta_max 必须满足 0 ≤ min < max ≤ π")
                intervals.append((lower, upper))
            intervals.sort()
            if any(right[0] < left[1] for left, right in zip(intervals, intervals[1:])):
                raise ValueError("2D 外表面分区不能重叠")
            if intervals and intervals[0][0] == 0 and intervals[-1][1] == math.pi and all(left[1] == right[0] for left, right in zip(intervals, intervals[1:])):
                raise ValueError("2D residual_zone 不能是空分区")
        else:
            directions = []
            for zone in zones.zone:
                values = (zone.theta, zone.phi, zone.angular_distance_min, zone.angular_distance_max)
                if any(value is None or not math.isfinite(value) for value in values):
                    raise ValueError(f"3D 分区 {zone.name}: 必须填写有限的方向和角距离")
                theta, phi, lower, upper = values
                if not 0 <= theta <= math.pi or not 0 <= phi < 2 * math.pi or not 0 <= lower < upper <= math.pi:
                    raise ValueError(f"3D 分区 {zone.name}: 角度超出有效范围")
                direction = (math.sin(theta) * math.cos(phi), math.sin(theta) * math.sin(phi), math.cos(theta))
                for previous, previous_direction in directions:
                    cross = (direction[1] * previous_direction[2] - direction[2] * previous_direction[1],
                             direction[2] * previous_direction[0] - direction[0] * previous_direction[2],
                             direction[0] * previous_direction[1] - direction[1] * previous_direction[0])
                    separation = math.atan2(math.hypot(*cross), sum(a * b for a, b in zip(direction, previous_direction)))
                    # 本分区方向到前一个环带的最近/最远角距离。
                    nearest = max(previous.angular_distance_min - separation, separation - previous.angular_distance_max, 0.0)
                    opposite = math.pi - separation
                    farthest = math.pi - max(previous.angular_distance_min - opposite, opposite - previous.angular_distance_max, 0.0)
                    if min(upper, farthest) - max(lower, nearest) > 1e-12:
                        raise ValueError(f"3D 外表面分区不能重叠: {previous.name}, {zone.name}")
                directions.append((zone, direction))
            if sum(math.cos(zone.angular_distance_min) - math.cos(zone.angular_distance_max) for zone in zones.zone) >= 2 - 1e-12:
                raise ValueError("3D residual_zone 不能是空分区")

    @staticmethod
    def check_number(value: float | None, name: str, *, allow_zero: bool = False) -> None:
        """检查必需的有限正数或非负数"""
        if value is None or not math.isfinite(value) or value < 0 or value == 0 and not allow_zero:
            requirement = "非负有限数" if allow_zero else "正有限数"
            raise ValueError(f"{name} 必须为{requirement}")

    @staticmethod
    def check_layers(layers: list) -> None:
        """区域与界面名称必须唯一, 避免生成拓扑映射时覆盖"""
        if not layers:
            raise ValueError("至少需要一个材料层")
        names = [layer.name for layer in layers]
        boundaries = [layer.outer_boundary.name for layer in layers]
        if len(set(names)) != len(names):
            raise ValueError("材料层名称不能重复")
        if len(set(boundaries)) != len(boundaries):
            raise ValueError("外界面名称不能重复")

    @classmethod
    def check_ideal_layers(cls, layers: list) -> None:
        """理想颗粒的外半径从内到外严格递增"""
        previous_radius = 0.0
        for layer in layers:
            cls.check_number(layer.outer_radius, f"{layer.name}.outer_radius")
            if layer.outer_radius <= previous_radius:
                raise ValueError("各层 outer_radius 必须从内到外严格递增")
            previous_radius = layer.outer_radius

    @classmethod
    def check_stochastic_geometry(cls, geometry: GeometryMeshCard.Geometry.StochasticMultilayerParticle, dimension: int) -> None:
        """检查核心, 包覆层及外轮廓所需的统计参数"""
        surface = geometry.outer_surface
        cls.check_number(surface.batch_mean_radius, "outer_surface.batch_mean_radius")
        if dimension > 1:
            if surface.target_sphericity is None or not 0 < surface.target_sphericity <= 1:
                raise ValueError("目标球形度必须在 (0,1] 内")
            expansion = surface.shape_expansion
            if expansion is None or expansion.max_degree < 2:
                raise ValueError("max_degree 必须至少为 2")
            cls.check_number(expansion.spectrum_decay, "spectrum_decay")
        layers = geometry.layers.layer
        core = layers[0]
        cls.check_number(core.batch_mean_radius, f"{core.name}.batch_mean_radius")
        cls.check_number(core.batch_radius_std, f"{core.name}.batch_radius_std", allow_zero=True)
        for layer in layers[1:]:
            cls.check_number(layer.batch_mean_thickness, f"{layer.name}.batch_mean_thickness")
            cls.check_number(layer.batch_thickness_std, f"{layer.name}.batch_thickness_std", allow_zero=True)
            if dimension > 1:
                cls.check_number(layer.within_particle_thickness_std, f"{layer.name}.within_particle_thickness_std", allow_zero=True)
                cls.check_number(layer.within_particle_radius_std, f"{layer.name}.within_particle_radius_std")
                coefficient = layer.unwrapping_coefficient
                if coefficient is None or not 0 <= coefficient <= 1:
                    raise ValueError(f"{layer.name}.unwrapping_coefficient 必须在 [0,1] 内")
        means = [core.batch_mean_radius] + [layer.batch_mean_thickness for layer in layers[1:]]
        deviations = [core.batch_radius_std] + [layer.batch_thickness_std for layer in layers[1:]]
        if all(value == 0 for value in deviations) and not math.isclose(sum(means), surface.batch_mean_radius, rel_tol=1e-12, abs_tol=1e-18):
            raise ValueError("批次标准差全为零时, 各层平均尺寸之和必须等于平均外半径")

    @classmethod
    def check_mesh(cls, mesh: GeometryMeshCard.Mesh, dimension: int, region_names: set[str]) -> None:
        """检查网格尺寸, 区域引用和算法"""
        if mesh.element.geometry_order not in (1, 2):
            raise ValueError("geometry_order 必须为 [1,2]")
        sampler = mesh.space_sampler
        if sampler.theta_interval_count is not None and sampler.theta_interval_count < 2:
            raise ValueError("theta_interval_count 必须至少为 2")
        if sampler.phi_count is not None and sampler.phi_count < 3:
            raise ValueError("phi_count 必须至少为 3")
        global_sizing = mesh.sizing.global_
        for name in ("min_edge_length", "max_edge_length"):
            value = getattr(global_sizing, name)
            if value is not None:
                cls.check_number(value, name)
        minimum, maximum = global_sizing.min_edge_length, global_sizing.max_edge_length
        if minimum is not None and maximum is not None and minimum > maximum:
            raise ValueError("min_edge_length 不能大于 max_edge_length")
        configured_regions = set()
        for region in mesh.sizing.region:
            if region.name not in region_names:
                raise ValueError(f"网格尺寸区域不存在: {region.name}")
            if region.name in configured_regions:
                raise ValueError(f"网格尺寸区域重复配置: {region.name}")
            configured_regions.add(region.name)
            cls.check_number(region.target_edge_length, f"{region.name}.target_edge_length")
        if dimension == 3 and mesh.gmsh.algorithm_3d not in ("hxt", "mmg3d", "del3d", "front3d"):
            raise ValueError("algorithm_3d 必须为 [hxt,mmg3d,del3d,front3d]")
        if mesh.gmsh.optimizer not in ("gmsh", "netgen", "none"):
            raise ValueError("optimizer 必须为 [gmsh,netgen,none]")
        for format_name in mesh.output.formats:
            if not format_name.isalnum():
                raise ValueError("网格输出格式应为不带点号的扩展名, 如 vtk bdf inp")
