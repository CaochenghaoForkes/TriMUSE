from trimuse.input.geometry_mesh.config import GeometryMeshCard
StochasticMultilayerParticleConfig = GeometryMeshCard.Geometry.StochasticMultilayerParticle
ZonesConfig = GeometryMeshCard.Geometry.Zones
SpaceSamplerConfig = GeometryMeshCard.Mesh.SpaceSampler
from trimuse.geometry.topology import GeometryTopology
from trimuse.geometry.zones import partition_surface_3d
import gmsh
import numpy as np
from scipy.special import sph_harm_y

class StochasticMultilayerParticleBuilder:
    """拆包裹法随机多层颗粒"""

    @classmethod
    def build(cls, config: StochasticMultilayerParticleConfig, dimension: int, rng: np.random.Generator, zones: ZonesConfig, space_sampler: SpaceSamplerConfig | None = None) -> GeometryTopology:
        """构造已通过输入检查的随机颗粒, Gmsh 生命周期由外部管理"""
        if dimension == 1:
            return cls._build_1d(config, rng, zones)
        if dimension == 2:
            return cls._build_2d(config, rng, zones, space_sampler)
        if dimension == 3:
            return cls._build_3d(config, rng, zones, space_sampler)

    @staticmethod
    def _batch_mean_thickness_cond(T: np.ndarray, S: np.ndarray, r_remaining: float) -> float:
        """计算当前最外层的条件均值, 数组包含从核心到当前层的未抽样部分"""
        # T 和 S 按从内到外排列, 末项对应当前层 l
        sum_S2 = float(np.sum(S ** 2))
        sum_T = float(np.sum(T))
        if sum_S2 == 0.0:
            return float(T[-1])
        if len(T) == 1:
            return r_remaining
        return float(T[-1] + S[-1] ** 2 / sum_S2 * (r_remaining - sum_T))

    @staticmethod
    def _batch_thickness_std_cond(S: np.ndarray) -> float:
        """计算当前最外层的条件标准差, 只计入尚未抽样的层"""
        sum_S2 = float(np.sum(S ** 2))
        if sum_S2 == 0.0:
            return 0.0
        # 用其他未抽样层的方差和避免一减去接近一的数造成精度损失
        sum_S2_inner = float(np.sum(S[:-1] ** 2))
        return float(S[-1] * np.sqrt(sum_S2_inner / sum_S2))

    @classmethod
    def _sample_mean_thicknesses(cls, config: StochasticMultilayerParticleConfig, rng: np.random.Generator, max_attempts: int = 1000) -> np.ndarray:
        """条件抽样各层平均尺寸, 返回从内到外的米制数组, 首项为核心半径"""
        # T 为批次均值, S 为批次标准差, t_bar 为本颗粒抽样平均尺寸
        region_configs = config.regions.region
        T = np.array([region_config.batch_mean_radius if l == 0 else region_config.batch_mean_thickness for l, region_config in enumerate(region_configs)])
        S = np.array([region_config.batch_radius_std if l == 0 else region_config.batch_thickness_std for l, region_config in enumerate(region_configs)])
        r_outer_bar = config.outer_surface.batch_mean_radius
        t_bar = np.empty(len(region_configs))
        for _ in range(max_attempts):
            for l in range(len(region_configs) - 1, -1, -1):
                r_remaining = r_outer_bar - float(np.sum(t_bar[l + 1:]))
                T_l_cond = cls._batch_mean_thickness_cond(T[:l + 1], S[:l + 1], r_remaining)
                S_l_cond = cls._batch_thickness_std_cond(S[:l + 1])
                t_bar[l] = T_l_cond if S_l_cond == 0.0 else rng.normal(T_l_cond, S_l_cond)
            if np.all(t_bar > 0) and np.all(np.isfinite(t_bar)):
                return t_bar
        raise RuntimeError(f"连续 {max_attempts} 次未抽到全部为正的尺寸, 请检查外半径和批次统计参数")

    @classmethod
    def _generate_outer_surface_2d(cls, config: StochasticMultilayerParticleConfig, rng: np.random.Generator, tolerance: float = 1e-8, max_attempts: int = 100) -> np.ndarray:
        """生成轴对称母线的米制勒让德系数, 常数项为球面平均半径"""
        surface = config.outer_surface
        R = surface.batch_mean_radius
        Psi_target = surface.target_sphericity
        N = surface.shape_expansion.max_degree
        q = surface.shape_expansion.spectrum_decay
        a = np.zeros(N + 1)
        a[0] = R
        if Psi_target == 1:
            return a
        cos_theta, weights = np.polynomial.legendre.leggauss(max(64, 4 * N))
        theta = np.arccos(cos_theta)
        n = np.arange(2, N + 1)
        E_n = (n + 1.0) ** (-q)
        for _ in range(max_attempts):
            a[2:] = R * np.sqrt(E_n) * rng.normal(size=len(n))
            r, dr_dtheta = cls._evaluate_legendre_2d(a, theta)
            # |P_n| 不超过 1, 由系数绝对值和限制连续母线的最小半径
            lambda_max = 0.99 * R / np.sum(np.abs(a[2:]))
            lambda_low = 0.0
            for lambda_high in np.linspace(0.0, lambda_max, 65)[1:]:
                Psi = cls._calculate_sphericity_2d(R + lambda_high * (r - R), lambda_high * dr_dtheta, weights)
                if Psi <= Psi_target:
                    break
                lambda_low = lambda_high
            else:
                continue
            for _ in range(80):
                lambda_mid = (lambda_low + lambda_high) / 2
                Psi = cls._calculate_sphericity_2d(R + lambda_mid * (r - R), lambda_mid * dr_dtheta, weights)
                if abs(Psi - Psi_target) <= tolerance:
                    a[2:] *= lambda_mid
                    return a
                if Psi > Psi_target:
                    lambda_low = lambda_mid
                else:
                    lambda_high = lambda_mid
        raise RuntimeError("未找到满足目标球形度及正半径约束的轴对称母线, 请调整球形度或频谱参数")

    @classmethod
    def _generate_layer_radial_profiles_2d(cls, config: StochasticMultilayerParticleConfig, t_bar: np.ndarray, a_outer: np.ndarray, rng: np.random.Generator, max_attempts: int = 1000) -> np.ndarray:
        """按拆包裹法生成从内到外各层母线的米制勒让德系数"""
        region_configs = config.regions.region
        layer_count = len(region_configs)
        if layer_count == 1:
            return a_outer[None, :].copy()

        coefficient_count = len(a_outer)
        for _ in range(max_attempts):
            a_layers = np.zeros((layer_count, coefficient_count))
            a_layers[-1] = a_outer
            for l in range(layer_count - 1, 0, -1):
                region_config = region_configs[l]
                sigma_t = region_config.within_particle_thickness_std
                sigma_r = region_config.within_particle_radius_std
                u_l = region_config.unwrapping_coefficient

                P_l = u_l / (u_l + np.sqrt(1 - u_l ** 2)) if u_l > 0 else 0.0
                normalization = np.sqrt(1 - 2 * P_l + 2 * P_l ** 2)
                a_thickness = np.zeros(coefficient_count)
                a_thickness[0] = t_bar[l]
                for n in range(1, coefficient_count):
                    radius_coefficient = a_layers[l, n]
                    random_coefficient = rng.choice((-1.0, 1.0)) * abs(radius_coefficient)
                    a_thickness[n] = sigma_t / sigma_r * (P_l * radius_coefficient + (1 - P_l) * random_coefficient) / normalization
                a_layers[l - 1] = a_layers[l] - a_thickness

            radius_lower_bounds = a_layers[:, 0] - np.sum(np.abs(a_layers[:, 1:]), axis=1)
            a_thickness_layers = a_layers[1:] - a_layers[:-1]
            thickness_lower_bounds = a_thickness_layers[:, 0] - np.sum(np.abs(a_thickness_layers[:, 1:]), axis=1)
            if np.all(radius_lower_bounds > 0) and np.all(thickness_lower_bounds > 0):
                return a_layers
            if all(cls._minimum_legendre_2d(a) > 0 for a in (*a_layers, *a_thickness_layers)):
                return a_layers
        raise RuntimeError(f"连续 {max_attempts} 次未生成处处为正且互不相交的二维轴对称分层母线, 请检查局部统计参数")

    @staticmethod
    def _evaluate_legendre_2d(a: np.ndarray, theta: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
        """计算轴对称半径 r(theta) 及其极角导数"""
        cos_theta = np.cos(theta)
        r = np.polynomial.legendre.legval(cos_theta, a)
        dr_dtheta = -np.sin(theta) * np.polynomial.legendre.legval(cos_theta, np.polynomial.legendre.legder(a))
        return r, dr_dtheta

    @staticmethod
    def _minimum_legendre_2d(a: np.ndarray) -> float:
        """由端点及导数实根求母线在整个极角区间的最小半径"""
        roots = np.polynomial.legendre.legroots(np.polynomial.legendre.legder(a))
        real_roots = roots[np.abs(roots.imag) < 1e-10].real
        coordinates = np.concatenate(([-1.0, 1.0], real_roots[(real_roots > -1) & (real_roots < 1)]))
        return float(np.min(np.polynomial.legendre.legval(coordinates, a)))

    @staticmethod
    def _calculate_sphericity_2d(r: np.ndarray, dr_dtheta: np.ndarray, weights: np.ndarray) -> float:
        """由轴对称母线旋转后的体积和表面积计算三维球形度"""
        V = 2 * np.pi / 3 * np.sum(weights * r ** 3)
        A = 2 * np.pi * np.sum(weights * r * np.hypot(r, dr_dtheta))
        return float(np.pi ** (1 / 3) * (6 * V) ** (2 / 3) / A)

    @classmethod
    def _generate_outer_surface_3d(cls, config: StochasticMultilayerParticleConfig, rng: np.random.Generator, tolerance: float = 1e-8, max_attempts: int = 100) -> np.ndarray:
        """生成米制实球谐系数 a_nm, 第二个下标使用 m 加 N"""
        surface = config.outer_surface
        R = surface.batch_mean_radius
        Psi_target = surface.target_sphericity
        N = surface.shape_expansion.max_degree
        q = surface.shape_expansion.spectrum_decay
        a_nm = np.zeros((N + 1, 2 * N + 1))
        a_nm[0, N] = np.sqrt(4 * np.pi) * R
        if Psi_target == 1:
            return a_nm
        theta, phi, theta_weights = cls._spherical_quadrature_3d(N)
        n_values = np.arange(2, N + 1)
        E_n = (n_values + 1.0) ** (-q)
        for _ in range(max_attempts):
            for n, energy in zip(n_values, E_n):
                g = rng.normal(size=2 * n + 1)
                norm = np.linalg.norm(g)
                if norm == 0:
                    break
                a_nm[n, N - n:N + n + 1] = R * np.sqrt(energy) * g / norm
            else:
                r, dr_dtheta, dr_dphi = cls._evaluate_spherical_harmonics_3d(a_nm, theta, phi)
                # 加法定理和柯西不等式保证连续球面上的半径为正
                harmonic_bound = sum(np.linalg.norm(a_nm[n, N - n:N + n + 1]) * np.sqrt((2 * n + 1) / (4 * np.pi)) for n in n_values)
                lambda_max = 0.99 * R / harmonic_bound
                lambda_low = 0.0
                for lambda_high in np.linspace(0.0, lambda_max, 65)[1:]:
                    Psi = cls._calculate_sphericity_3d(R + lambda_high * (r - R), lambda_high * dr_dtheta, lambda_high * dr_dphi, theta, theta_weights)
                    if Psi <= Psi_target:
                        break
                    lambda_low = lambda_high
                else:
                    continue
                for _ in range(80):
                    lambda_mid = (lambda_low + lambda_high) / 2
                    Psi = cls._calculate_sphericity_3d(R + lambda_mid * (r - R), lambda_mid * dr_dtheta, lambda_mid * dr_dphi, theta, theta_weights)
                    if abs(Psi - Psi_target) <= tolerance:
                        a_nm[2:] *= lambda_mid
                        return a_nm
                    if Psi > Psi_target:
                        lambda_low = lambda_mid
                    else:
                        lambda_high = lambda_mid
        raise RuntimeError("未找到满足目标球形度及正半径约束的表面, 请调整球形度或频谱参数")

    @classmethod
    def _generate_layer_radial_profiles_3d(cls, config: StochasticMultilayerParticleConfig, t_bar: np.ndarray, a_nm_outer: np.ndarray, rng: np.random.Generator, max_attempts: int = 1000) -> np.ndarray:
        """按拆包裹法生成从内到外各层界面的米制实球谐系数"""
        region_configs = config.regions.region
        layer_count = len(region_configs)
        N = len(a_nm_outer) - 1
        if layer_count == 1:
            return a_nm_outer[None, :, :].copy()
        theta_check, phi_check, _ = cls._spherical_quadrature_3d(2 * N)

        for _ in range(max_attempts):
            a_nm_layers = np.zeros((layer_count, N + 1, 2 * N + 1))
            a_nm_layers[-1] = a_nm_outer
            valid_sample = True
            for l in range(layer_count - 1, 0, -1):
                region_config = region_configs[l]
                sigma_t = region_config.within_particle_thickness_std
                sigma_r = region_config.within_particle_radius_std
                u_l = region_config.unwrapping_coefficient

                P_l = u_l / (u_l + np.sqrt(1 - u_l ** 2)) if u_l > 0 else 0.0
                normalization = np.sqrt(1 - 2 * P_l + 2 * P_l ** 2)
                thickness_coefficients = np.zeros_like(a_nm_outer)
                thickness_coefficients[0, N] = np.sqrt(4 * np.pi) * t_bar[l]
                for n in range(1, N + 1):
                    coefficient_slice = slice(N - n, N + n + 1)
                    radius_coefficients = a_nm_layers[l, n, coefficient_slice]
                    coefficient_magnitude = float(np.sum(np.abs(radius_coefficients)))
                    if coefficient_magnitude == 0.0:
                        random_coefficients = np.zeros(2 * n + 1)
                    else:
                        random_coefficients = rng.uniform(-1.0, 1.0, 2 * n + 1)
                        random_magnitude = float(np.sum(np.abs(random_coefficients)))
                        if random_magnitude == 0.0:
                            valid_sample = False
                            break
                        random_coefficients *= coefficient_magnitude / random_magnitude
                    thickness_coefficients[n, coefficient_slice] = sigma_t / sigma_r * (P_l * radius_coefficients + (1 - P_l) * random_coefficients) / normalization
                if not valid_sample:
                    break
                a_nm_layers[l - 1] = a_nm_layers[l] - thickness_coefficients
            if not valid_sample:
                continue

            radius_lower_bounds = np.empty(layer_count)
            for l in range(layer_count):
                harmonic_bound = sum(np.linalg.norm(a_nm_layers[l, n, N - n:N + n + 1]) * np.sqrt((2 * n + 1) / (4 * np.pi)) for n in range(1, N + 1))
                radius_lower_bounds[l] = a_nm_layers[l, 0, N] / np.sqrt(4 * np.pi) - harmonic_bound
            thickness_layers = a_nm_layers[1:] - a_nm_layers[:-1]
            thickness_lower_bounds = np.empty(layer_count - 1)
            for l in range(layer_count - 1):
                harmonic_bound = sum(np.linalg.norm(thickness_layers[l, n, N - n:N + n + 1]) * np.sqrt((2 * n + 1) / (4 * np.pi)) for n in range(1, N + 1))
                thickness_lower_bounds[l] = thickness_layers[l, 0, N] / np.sqrt(4 * np.pi) - harmonic_bound
            if np.all(radius_lower_bounds > 0) and np.all(thickness_lower_bounds > 0):
                return a_nm_layers
            radial_profiles = np.array([cls._evaluate_spherical_harmonics_3d(coefficients, theta_check, phi_check)[0] for coefficients in a_nm_layers])
            if np.all(radial_profiles > 0) and np.all(np.diff(radial_profiles, axis=0) > 0):
                return a_nm_layers
        raise RuntimeError(f"连续 {max_attempts} 次未生成处处为正且互不相交的三维分层表面, 请检查局部统计参数")

    @staticmethod
    def _spherical_quadrature_3d(N: int) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
        """建立极角高斯积分点和方位角等距积分点"""
        cos_theta, theta_weights = np.polynomial.legendre.leggauss(max(64, 4 * N))
        theta = np.arccos(cos_theta)
        phi = np.linspace(0.0, 2 * np.pi, max(128, 8 * N), endpoint=False)
        return theta, phi, theta_weights

    @staticmethod
    def _evaluate_spherical_harmonics_3d(a_nm: np.ndarray, theta: np.ndarray, phi: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
        """计算 r(theta,phi) 及其对极角和方位角的导数"""
        N = len(a_nm) - 1
        r = np.zeros((len(theta), len(phi)))
        dr_dtheta = np.zeros_like(r)
        dr_dphi = np.zeros_like(r)
        for n in range(N + 1):
            Y, dY = sph_harm_y(n, 0, theta, 0.0, diff_n=1)
            coefficient = a_nm[n, N]
            r += coefficient * Y.real[:, None]
            dr_dtheta += coefficient * dY[..., 0].real[:, None]
            for m in range(1, n + 1):
                Y, dY = sph_harm_y(n, m, theta, 0.0, diff_n=1)
                cosine_coefficient = a_nm[n, N + m]
                sine_coefficient = a_nm[n, N - m]
                cos_m_phi, sin_m_phi = np.cos(m * phi), np.sin(m * phi)
                angular_part = cosine_coefficient * cos_m_phi + sine_coefficient * sin_m_phi
                angular_derivative = m * (-cosine_coefficient * sin_m_phi + sine_coefficient * cos_m_phi)
                r += np.sqrt(2) * Y.real[:, None] * angular_part
                dr_dtheta += np.sqrt(2) * dY[..., 0].real[:, None] * angular_part
                dr_dphi += np.sqrt(2) * Y.real[:, None] * angular_derivative
        return r, dr_dtheta, dr_dphi

    @staticmethod
    def _calculate_sphericity_3d(r: np.ndarray, dr_dtheta: np.ndarray, dr_dphi: np.ndarray, theta: np.ndarray, theta_weights: np.ndarray) -> float:
        """利用极角高斯积分和方位角等距积分计算球形度"""
        dphi = 2 * np.pi / r.shape[1]
        weights = theta_weights[:, None]
        V = dphi / 3 * np.sum(weights * r ** 3)
        metric = np.sqrt(1 + (dr_dtheta / r) ** 2 + (dr_dphi / (r * np.sin(theta)[:, None])) ** 2)
        A = dphi * np.sum(weights * r ** 2 * metric)
        return float(np.pi ** (1 / 3) * (6 * V) ** (2 / 3) / A)

    @staticmethod
    def _radial_to_cartesian_2d(r: np.ndarray, theta: np.ndarray, center: GeometryMeshCard.Geometry.Center) -> np.ndarray:
        """将轴对称母线转换到右半平面, 旋转轴为 x 等于中心 x"""
        x = center.x + r * np.sin(theta)
        x[..., 0], x[..., -1] = center.x, center.x
        y = center.y + r * np.cos(theta)
        return np.stack((x, y, np.zeros_like(r)), axis=-1)

    @staticmethod
    def _radial_to_cartesian_3d(r: np.ndarray, theta: np.ndarray, phi: np.ndarray, center: GeometryMeshCard.Geometry.Center) -> np.ndarray:
        """将三维径向表面转换为最后一维按 x, y, z 排列的笛卡尔坐标"""
        center_coordinates = np.array((center.x, center.y, center.z))
        sin_theta = np.sin(theta)[:, None]
        x = center_coordinates[0] + r * sin_theta * np.cos(phi)[None, :]
        y = center_coordinates[1] + r * sin_theta * np.sin(phi)[None, :]
        z = center_coordinates[2] + r * np.cos(theta)[:, None]
        return np.stack((x, y, z), axis=-1)

    @classmethod
    def _build_1d(cls, config: StochasticMultilayerParticleConfig, rng: np.random.Generator, zones: ZonesConfig) -> GeometryTopology:
        """按抽样平均厚度构造径向线, Gmsh 生命周期由外部管理"""
        t_bar = cls._sample_mean_thicknesses(config, rng)
        r_bar = np.cumsum(t_bar)
        center_x = config.center.x
        points = [gmsh.model.occ.addPoint(center_x, 0.0, 0.0)]
        for r_l_bar in r_bar:
            points.append(gmsh.model.occ.addPoint(center_x + float(r_l_bar), 0.0, 0.0))
        regions = {}
        boundaries = {"particle_center": [points[0]]}
        for l, region_config in enumerate(config.regions.region):
            line = gmsh.model.occ.addLine(points[l], points[l + 1])
            regions[region_config.name] = [line]
            boundary_name = zones.residual_zone if l == len(config.regions.region) - 1 else region_config.outer_boundary.name
            boundaries[boundary_name] = [points[l + 1]]
        gmsh.model.occ.synchronize()
        return GeometryTopology(dimension=1, regions=regions, boundaries=boundaries)

    @classmethod
    def _build_2d(cls, config: StochasticMultilayerParticleConfig, rng: np.random.Generator, zones: ZonesConfig, space_sampler: SpaceSamplerConfig | None = None) -> GeometryTopology:
        """构造二维轴对称随机颗粒右半截面"""
        t_bar = cls._sample_mean_thicknesses(config, rng)
        a_outer = cls._generate_outer_surface_2d(config, rng)
        a_layers = cls._generate_layer_radial_profiles_2d(config, t_bar, a_outer, rng)

        N = len(a_outer) - 1
        sampler = space_sampler if space_sampler is not None else SpaceSamplerConfig()
        theta_interval_count = sampler.theta_interval_count if sampler.theta_interval_count is not None else max(128, 8 * N)
        cut_angles = sorted({0.0, np.pi, *(angle for zone in zones.zone for angle in (zone.theta_min, zone.theta_max))})
        theta = np.unique(np.r_[np.linspace(0.0, np.pi, theta_interval_count + 1), cut_angles])
        radial_profiles = np.array([cls._evaluate_legendre_2d(a, theta)[0] for a in a_layers])
        coordinates = cls._radial_to_cartesian_2d(radial_profiles, theta, config.center)

        curve_tags = []
        top_points = []
        bottom_points = []
        outer_segments = []
        for layer_index, layer_coordinates in enumerate(coordinates):
            point_tags = [gmsh.model.occ.addPoint(float(x), float(y), float(z)) for x, y, z in layer_coordinates]
            top_points.append(point_tags[0])
            bottom_points.append(point_tags[-1])
            if layer_index == len(coordinates) - 1:
                cut_indices = [int(np.searchsorted(theta, angle)) for angle in cut_angles]
                outer_segments = [(gmsh.model.occ.addSpline(point_tags[start:end + 1]), (cut_angles[index] + cut_angles[index + 1]) / 2) for index, (start, end) in enumerate(zip(cut_indices, cut_indices[1:]))]
                curve_tags.append([tag for tag, _ in outer_segments])
            else:
                curve_tags.append([gmsh.model.occ.addSpline(point_tags)])

        center_tag = gmsh.model.occ.addPoint(config.center.x, config.center.y, 0.0)
        axis_tags = [gmsh.model.occ.addLine(top_points[0], center_tag), gmsh.model.occ.addLine(center_tag, bottom_points[0])]
        kernel_loop = gmsh.model.occ.addCurveLoop([*curve_tags[0], -axis_tags[1], -axis_tags[0]])
        surface_tags = [gmsh.model.occ.addPlaneSurface([kernel_loop])]
        for l in range(1, len(curve_tags)):
            bottom_axis = gmsh.model.occ.addLine(bottom_points[l], bottom_points[l - 1])
            top_axis = gmsh.model.occ.addLine(top_points[l - 1], top_points[l])
            axis_tags.extend((bottom_axis, top_axis))
            loop = gmsh.model.occ.addCurveLoop([*curve_tags[l], bottom_axis, *[-tag for tag in reversed(curve_tags[l - 1])], top_axis])
            surface_tags.append(gmsh.model.occ.addPlaneSurface([loop]))
        gmsh.model.occ.synchronize()

        regions = {region_config.name: [surface_tags[l]] for l, region_config in enumerate(config.regions.region)}
        boundaries = {region_config.outer_boundary.name: curve_tags[l] for l, region_config in enumerate(config.regions.region[:-1])}
        boundaries.update({zone.name: [] for zone in zones.zone})
        boundaries[zones.residual_zone] = []
        for tag, angle in outer_segments:
            name = next((zone.name for zone in zones.zone if zone.theta_min <= angle <= zone.theta_max), zones.residual_zone)
            boundaries[name].append(tag)
        boundaries["symmetry_axis"] = axis_tags
        return GeometryTopology(dimension=2, regions=regions, boundaries=boundaries)

    @classmethod
    def _build_3d(cls, config: StochasticMultilayerParticleConfig, rng: np.random.Generator, zones: ZonesConfig, space_sampler: SpaceSamplerConfig | None = None) -> GeometryTopology:
        """构造三维随机多层颗粒, Gmsh 生命周期由外部管理"""
        t_bar = cls._sample_mean_thicknesses(config, rng)
        a_nm_outer = cls._generate_outer_surface_3d(config, rng)
        a_nm_layers = cls._generate_layer_radial_profiles_3d(config, t_bar, a_nm_outer, rng)

        N = len(a_nm_outer) - 1
        sampler = space_sampler if space_sampler is not None else SpaceSamplerConfig()
        theta_interval_count = sampler.theta_interval_count if sampler.theta_interval_count is not None else max(16, N + 1)
        phi_count = sampler.phi_count if sampler.phi_count is not None else max(32, 2 * N + 1)
        theta = np.linspace(0.0, np.pi, theta_interval_count + 1)
        phi = np.linspace(0.0, 2 * np.pi, phi_count, endpoint=False)
        radial_profiles = np.array([cls._evaluate_spherical_harmonics_3d(a_nm, theta, phi)[0] for a_nm in a_nm_layers])
        # 单位尺度下构造并切分 CAD，最后统一恢复尺寸，保持小面片的数值精度。
        scale = config.outer_surface.batch_mean_radius
        origin = GeometryMeshCard.Geometry.Center()
        coordinates = cls._radial_to_cartesian_3d(radial_profiles / scale, theta, phi, origin)

        interface_surfaces = []
        shell_tags = []
        outer_radius = float(np.linalg.norm(coordinates[-1], axis=-1).max())
        for layer_index, layer_coordinates in enumerate(coordinates):
            point_tags = np.empty((len(theta), len(phi)), dtype=int)
            north_tag = gmsh.model.occ.addPoint(*map(float, layer_coordinates[0, 0]))
            south_tag = gmsh.model.occ.addPoint(*map(float, layer_coordinates[-1, 0]))
            point_tags[0, :] = north_tag
            point_tags[-1, :] = south_tag
            for i in range(1, len(theta) - 1):
                point_tags[i] = [gmsh.model.occ.addPoint(*map(float, point)) for point in layer_coordinates[i]]

            triangles = []
            for j in range(len(phi)):
                j_next = (j + 1) % len(phi)
                triangles.append((point_tags[0, j], point_tags[1, j], point_tags[1, j_next]))
                for i in range(1, len(theta) - 2):
                    p_00, p_10 = point_tags[i, j], point_tags[i + 1, j]
                    p_11, p_01 = point_tags[i + 1, j_next], point_tags[i, j_next]
                    triangles.extend(((p_00, p_10, p_11), (p_00, p_11, p_01)))
                triangles.append((point_tags[-2, j], point_tags[-1, j], point_tags[-2, j_next]))

            edge_tags = {}
            surface_tags = []
            for triangle in triangles:
                oriented_edges = []
                for start_tag, end_tag in zip(triangle, triangle[1:] + triangle[:1]):
                    edge_key = (min(start_tag, end_tag), max(start_tag, end_tag))
                    if edge_key not in edge_tags:
                        edge_tags[edge_key] = gmsh.model.occ.addLine(*edge_key)
                    edge_tag = edge_tags[edge_key]
                    oriented_edges.append(edge_tag if start_tag < end_tag else -edge_tag)
                curve_loop_tag = gmsh.model.occ.addCurveLoop(oriented_edges)
                surface_tags.append(gmsh.model.occ.addPlaneSurface([curve_loop_tag]))
            if layer_index == len(coordinates) - 1:
                outer_boundaries = partition_surface_3d(surface_tags, zones, outer_radius)
                surface_tags = [tag for tags in outer_boundaries.values() for tag in tags]
            interface_surfaces.append(surface_tags)
            shell_tags.append(gmsh.model.occ.addSurfaceLoop(surface_tags))

        volume_tags = [gmsh.model.occ.addVolume([shell_tags[0]])]
        for l in range(1, len(shell_tags)):
            volume_tags.append(gmsh.model.occ.addVolume([shell_tags[l], shell_tags[l - 1]]))
        # 曲面放在体之前，变换后保留各层界面和分区的曲面编号。
        entities = [(2, tag) for tags in interface_surfaces for tag in tags] + [(3, tag) for tag in volume_tags]
        gmsh.model.occ.dilate(entities, 0.0, 0.0, 0.0, scale, scale, scale)
        gmsh.model.occ.translate(entities, config.center.x, config.center.y, config.center.z)
        gmsh.model.occ.synchronize()

        regions = {region_config.name: [volume_tags[l]] for l, region_config in enumerate(config.regions.region)}
        boundaries = {region_config.outer_boundary.name: interface_surfaces[l] for l, region_config in enumerate(config.regions.region[:-1])}
        boundaries.update(outer_boundaries)
        return GeometryTopology(dimension=3, regions=regions, boundaries=boundaries)

    
