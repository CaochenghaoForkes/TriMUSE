"""拆包裹法随机多层颗粒几何构造器"""

from trimuse.input.geometry_mesh_config import GeometryMeshCard
StochasticMultilayerParticleConfig = GeometryMeshCard.Geometry.StochasticMultilayerParticle
from trimuse.geometry.topology import GeometryTopology
import gmsh
import numpy as np
from scipy.special import sph_harm_y

class StochasticMultilayerParticleBuilder:
    """拆包裹法随机多层颗粒"""

    @classmethod
    def build(cls, config: StochasticMultilayerParticleConfig, dimension: int, rng: np.random.Generator) -> GeometryTopology:
        """构造拆包裹法随机多层颗粒"""
        if dimension == 1:
            return cls._build_1d(config, rng)
        if dimension == 2:
            return cls._build_2d(config, rng)
        if dimension == 3:
            return cls._build_3d(config, rng)
        raise ValueError("dimension 必须为 1, 2 或 3")

    @staticmethod
    def _batch_mean_thickness_cond(T: np.ndarray, S: np.ndarray, r_remaining: float) -> float:
        """计算当前最外层的条件均值, 数组包含从核心到当前层的未抽样部分"""
        # T 和 S 按从内到外排列, 末项对应当前层 l
        sum_S2 = float(np.sum(S ** 2))
        sum_T = float(np.sum(T))
        if sum_S2 == 0.0:
            if not np.isclose(r_remaining, sum_T, rtol=1e-12, atol=1e-18):
                raise ValueError("未抽样层的标准差全为零, 其平均尺寸之和与剩余半径不一致")
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
        layers = config.layers.layer
        if not layers:
            raise ValueError("至少需要一个核心层")
        T = np.array([layer.batch_mean_radius.value if l == 0 else layer.batch_mean_thickness.value for l, layer in enumerate(layers)])
        S = np.array([layer.batch_radius_std.value if l == 0 else layer.batch_thickness_std.value for l, layer in enumerate(layers)])
        r_outer_bar = config.outer_surface.batch_mean_radius.value
        if not np.isfinite(r_outer_bar) or r_outer_bar <= 0:
            raise ValueError("外半径必须为有限正数, 单位为米")
        if not np.all(np.isfinite(T)) or np.any(T <= 0) or not np.all(np.isfinite(S)) or np.any(S < 0):
            raise ValueError("平均尺寸必须为有限正数, 标准差必须为有限非负数")
        if type(max_attempts) is not int or max_attempts < 1:
            raise ValueError("max_attempts 必须为正整数")
        t_bar = np.empty(len(layers))
        for _ in range(max_attempts):
            for l in range(len(layers) - 1, -1, -1):
                r_remaining = r_outer_bar - float(np.sum(t_bar[l + 1:]))
                T_l_cond = cls._batch_mean_thickness_cond(T[:l + 1], S[:l + 1], r_remaining)
                S_l_cond = cls._batch_thickness_std_cond(S[:l + 1])
                t_bar[l] = T_l_cond if S_l_cond == 0.0 else rng.normal(T_l_cond, S_l_cond)
            if np.all(t_bar > 0) and np.all(np.isfinite(t_bar)):
                return t_bar
        raise RuntimeError(f"连续 {max_attempts} 次未抽到全部为正的尺寸, 请检查外半径和批次统计参数")

    @classmethod
    def _generate_outer_surface_2d(cls, config: StochasticMultilayerParticleConfig, rng: np.random.Generator, tolerance: float = 1e-8, max_attempts: int = 100) -> tuple[np.ndarray, np.ndarray]:
        """生成米制傅里叶系数 a 和 b, 常数项 a[0] 为平均外半径"""
        surface = config.outer_surface
        if surface.target_circularity is None or surface.shape_expansion is None:
            raise ValueError("二维外轮廓需要 target_circularity 和 shape_expansion")
        R = surface.batch_mean_radius.value
        C_target = surface.target_circularity.value
        N = surface.shape_expansion.max_degree
        q = surface.shape_expansion.spectrum_decay
        if not np.isfinite(R) or R <= 0 or not 0 < C_target <= 1:
            raise ValueError("平均外半径必须为有限正数, 圆形度必须在 (0,1] 内")
        if type(N) is not int or N < 2 or not np.isfinite(q) or q <= 0:
            raise ValueError("展开次数至少为 2, 频谱衰减指数必须为有限正数")
        if not 0 < tolerance < 1 or type(max_attempts) is not int or max_attempts < 1:
            raise ValueError("容差必须在 (0,1) 内, 最大尝试次数必须为正整数")
        a, b = np.zeros(N + 1), np.zeros(N + 1)
        a[0] = R
        if C_target == 1:
            return a, b
        phi = np.linspace(0.0, 2 * np.pi, max(4096, 64 * N), endpoint=False)
        n = np.arange(2, N + 1)
        # 每阶总能量 E_n 与 (n+1) 的负 q 次幂成正比
        E_n = (n + 1.0) ** (-q)
        for _ in range(max_attempts):
            g = rng.normal(size=(2, N - 1))
            norm = np.linalg.norm(g, axis=0)
            if np.any(norm == 0):
                continue
            a[2:], b[2:] = R * np.sqrt(E_n) * g / norm
            r, dr_dphi = cls._evaluate_fourier_2d(a, b, phi)
            # 三角不等式保证整个圆周半径为正, 不仅是离散采样点
            lambda_max = 0.99 * R / np.sum(np.hypot(a[2:], b[2:]))
            lambda_low = 0.0
            for lambda_high in np.linspace(0.0, lambda_max, 65)[1:]:
                C = cls._calculate_circularity_2d(R + lambda_high * (r - R), lambda_high * dr_dphi)
                if C <= C_target:
                    break
                lambda_low = lambda_high
            else:
                continue
            for _ in range(80):
                lambda_mid = (lambda_low + lambda_high) / 2
                C = cls._calculate_circularity_2d(R + lambda_mid * (r - R), lambda_mid * dr_dphi)
                if abs(C - C_target) <= tolerance:
                    a[2:] *= lambda_mid
                    b[2:] *= lambda_mid
                    return a, b
                if C > C_target:
                    lambda_low = lambda_mid
                else:
                    lambda_high = lambda_mid
        raise RuntimeError("未找到满足目标圆形度及正半径约束的轮廓, 请调整圆形度或频谱参数")

    @classmethod
    def _generate_layer_radial_profiles_2d(cls, config: StochasticMultilayerParticleConfig, t_bar: np.ndarray, a_outer: np.ndarray, b_outer: np.ndarray, rng: np.random.Generator, max_attempts: int = 1000) -> tuple[np.ndarray, np.ndarray]:
        """按拆包裹法生成从内到外各层界面的米制傅里叶系数"""
        layers = config.layers.layer
        layer_count = len(layers)
        if layer_count == 0 or t_bar.shape != (layer_count,) or np.any(~np.isfinite(t_bar)) or np.any(t_bar <= 0):
            raise ValueError("t_bar 必须包含从内到外各层的有限正平均尺寸")
        if a_outer.ndim != 1 or b_outer.shape != a_outer.shape or len(a_outer) < 3:
            raise ValueError("外轮廓傅里叶系数必须是长度相同且至少包含二次项的一维数组")
        if np.any(~np.isfinite(a_outer)) or np.any(~np.isfinite(b_outer)):
            raise ValueError("外轮廓傅里叶系数必须为有限数")
        if not np.isclose(np.sum(t_bar), a_outer[0], rtol=1e-12, atol=1e-18):
            raise ValueError("各层平均尺寸之和必须等于外轮廓平均半径")
        if type(max_attempts) is not int or max_attempts < 1:
            raise ValueError("max_attempts 必须为正整数")
        if layer_count == 1:
            return a_outer[None, :].copy(), b_outer[None, :].copy()

        coefficient_count = len(a_outer)
        for _ in range(max_attempts):
            a_layers = np.zeros((layer_count, coefficient_count))
            b_layers = np.zeros_like(a_layers)
            a_layers[-1], b_layers[-1] = a_outer, b_outer
            valid_sample = True
            for l in range(layer_count - 1, 0, -1):
                layer = layers[l]
                sigma_t = layer.within_particle_thickness_std
                sigma_r = layer.within_particle_radius_std
                coefficient = layer.unwrapping_coefficient
                if sigma_t is None or sigma_r is None or coefficient is None:
                    raise ValueError(f"{layer.name} 缺少颗粒内厚度标准差, 半径标准差或拆包裹系数")
                sigma_t, sigma_r, u_l = sigma_t.value, sigma_r.value, coefficient.value
                if not np.isfinite(sigma_t) or sigma_t < 0 or not np.isfinite(sigma_r) or sigma_r <= 0 or not np.isfinite(u_l) or not 0 <= u_l <= 1:
                    raise ValueError(f"{layer.name} 的局部统计参数无效")

                P_l = u_l / (u_l + np.sqrt(1 - u_l ** 2)) if u_l > 0 else 0.0
                normalization = np.sqrt(1 - 2 * P_l + 2 * P_l ** 2)
                a_thickness = np.zeros(coefficient_count)
                b_thickness = np.zeros(coefficient_count)
                a_thickness[0] = t_bar[l]
                for n in range(1, coefficient_count):
                    radius_coefficients = np.array((a_layers[l, n], b_layers[l, n]))
                    coefficient_magnitude = float(np.sum(np.abs(radius_coefficients)))
                    if coefficient_magnitude == 0.0:
                        random_coefficients = np.zeros(2)
                    else:
                        random_coefficients = rng.uniform(-1.0, 1.0, 2)
                        random_magnitude = float(np.sum(np.abs(random_coefficients)))
                        if random_magnitude == 0.0:
                            valid_sample = False
                            break
                        random_coefficients *= coefficient_magnitude / random_magnitude
                    thickness_coefficients = sigma_t / sigma_r * (P_l * radius_coefficients + (1 - P_l) * random_coefficients) / normalization
                    a_thickness[n], b_thickness[n] = thickness_coefficients
                if not valid_sample:
                    break
                a_layers[l - 1] = a_layers[l] - a_thickness
                b_layers[l - 1] = b_layers[l] - b_thickness
            if not valid_sample:
                continue

            radius_lower_bounds = a_layers[:, 0] - np.sum(np.hypot(a_layers[:, 1:], b_layers[:, 1:]), axis=1)
            a_thickness_layers = a_layers[1:] - a_layers[:-1]
            b_thickness_layers = b_layers[1:] - b_layers[:-1]
            thickness_lower_bounds = a_thickness_layers[:, 0] - np.sum(np.hypot(a_thickness_layers[:, 1:], b_thickness_layers[:, 1:]), axis=1)
            if np.all(radius_lower_bounds > 0) and np.all(thickness_lower_bounds > 0):
                return a_layers, b_layers
        raise RuntimeError(f"连续 {max_attempts} 次未生成处处为正且互不相交的二维分层轮廓, 请检查局部统计参数")

    @staticmethod
    def _evaluate_fourier_2d(a: np.ndarray, b: np.ndarray, phi: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
        """计算 r(phi) 及角度导数, a[n] 对应余弦项, b[n] 对应正弦项"""
        n = np.arange(len(a))[:, None]
        cos_n_phi, sin_n_phi = np.cos(n * phi), np.sin(n * phi)
        r = a @ cos_n_phi + b @ sin_n_phi
        dr_dphi = -(n[:, 0] * a) @ sin_n_phi + (n[:, 0] * b) @ cos_n_phi
        return r, dr_dphi

    @staticmethod
    def _calculate_circularity_2d(r: np.ndarray, dr_dphi: np.ndarray) -> float:
        """由不重复端点的等角度采样计算圆形度, C 等于 4 pi A 除以 L 的平方"""
        dphi = 2 * np.pi / len(r)
        A = 0.5 * dphi * np.sum(r ** 2)
        L = dphi * np.sum(np.hypot(r, dr_dphi))
        return float(4 * np.pi * A / L ** 2)

    @classmethod
    def _generate_outer_surface_3d(cls, config: StochasticMultilayerParticleConfig, rng: np.random.Generator, tolerance: float = 1e-8, max_attempts: int = 100) -> np.ndarray:
        """生成米制实球谐系数 a_nm, 第二个下标使用 m 加 N"""
        surface = config.outer_surface
        if surface.target_sphericity is None or surface.shape_expansion is None:
            raise ValueError("三维外表面需要 target_sphericity 和 shape_expansion")
        R = surface.batch_mean_radius.value
        Psi_target = surface.target_sphericity.value
        N = surface.shape_expansion.max_degree
        q = surface.shape_expansion.spectrum_decay
        if not np.isfinite(R) or R <= 0 or not 0 < Psi_target <= 1:
            raise ValueError("平均外半径必须为有限正数, 球形度必须在 (0,1] 内")
        if type(N) is not int or N < 2 or not np.isfinite(q) or q <= 0:
            raise ValueError("展开次数至少为 2, 频谱衰减指数必须为有限正数")
        if not 0 < tolerance < 1 or type(max_attempts) is not int or max_attempts < 1:
            raise ValueError("容差必须在 (0,1) 内, 最大尝试次数必须为正整数")
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
        layers = config.layers.layer
        layer_count = len(layers)
        if layer_count == 0 or t_bar.shape != (layer_count,) or np.any(~np.isfinite(t_bar)) or np.any(t_bar <= 0):
            raise ValueError("t_bar 必须包含从内到外各层的有限正平均尺寸")
        if a_nm_outer.ndim != 2 or len(a_nm_outer) < 3 or a_nm_outer.shape[1] != 2 * len(a_nm_outer) - 1:
            raise ValueError("外表面实球谐系数必须具有形状 (N + 1, 2 * N + 1), 且 N 至少为 2")
        if np.any(~np.isfinite(a_nm_outer)):
            raise ValueError("外表面实球谐系数必须为有限数")
        N = len(a_nm_outer) - 1
        r_outer_bar = a_nm_outer[0, N] / np.sqrt(4 * np.pi)
        if not np.isclose(np.sum(t_bar), r_outer_bar, rtol=1e-12, atol=1e-18):
            raise ValueError("各层平均尺寸之和必须等于外表面平均半径")
        if type(max_attempts) is not int or max_attempts < 1:
            raise ValueError("max_attempts 必须为正整数")
        if layer_count == 1:
            return a_nm_outer[None, :, :].copy()
        theta_check, phi_check, _ = cls._spherical_quadrature_3d(2 * N)

        for _ in range(max_attempts):
            a_nm_layers = np.zeros((layer_count, N + 1, 2 * N + 1))
            a_nm_layers[-1] = a_nm_outer
            valid_sample = True
            for l in range(layer_count - 1, 0, -1):
                layer = layers[l]
                sigma_t = layer.within_particle_thickness_std
                sigma_r = layer.within_particle_radius_std
                coefficient = layer.unwrapping_coefficient
                if sigma_t is None or sigma_r is None or coefficient is None:
                    raise ValueError(f"{layer.name} 缺少颗粒内厚度标准差, 半径标准差或拆包裹系数")
                sigma_t, sigma_r, u_l = sigma_t.value, sigma_r.value, coefficient.value
                if not np.isfinite(sigma_t) or sigma_t < 0 or not np.isfinite(sigma_r) or sigma_r <= 0 or not np.isfinite(u_l) or not 0 <= u_l <= 1:
                    raise ValueError(f"{layer.name} 的局部统计参数无效")

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
        if r.shape != dr_dtheta.shape or r.shape != dr_dphi.shape or r.shape[0] != len(theta) or len(theta) != len(theta_weights):
            raise ValueError("球面采样数组的形状不一致")
        dphi = 2 * np.pi / r.shape[1]
        weights = theta_weights[:, None]
        V = dphi / 3 * np.sum(weights * r ** 3)
        metric = np.sqrt(1 + (dr_dtheta / r) ** 2 + (dr_dphi / (r * np.sin(theta)[:, None])) ** 2)
        A = dphi * np.sum(weights * r ** 2 * metric)
        return float(np.pi ** (1 / 3) * (6 * V) ** (2 / 3) / A)

    @staticmethod
    def _radial_to_cartesian_2d(r: np.ndarray, phi: np.ndarray, center: GeometryMeshCard.Geometry.IdealMultilayerSphere.Center) -> np.ndarray:
        """将二维径向轮廓转换为最后一维按 x, y, z 排列的笛卡尔坐标"""
        if not isinstance(r, np.ndarray) or r.ndim < 1 or not isinstance(phi, np.ndarray) or phi.ndim != 1 or r.shape[-1] != len(phi):
            raise ValueError("二维径向轮廓的最后一维必须与一维方位角数组长度相同")
        center_coordinates = np.array((center.x.value, center.y.value))
        if np.any(~np.isfinite(r)) or np.any(~np.isfinite(phi)) or np.any(~np.isfinite(center_coordinates)):
            raise ValueError("二维径向轮廓, 方位角和中心坐标必须为有限数")
        x = center_coordinates[0] + r * np.cos(phi)
        y = center_coordinates[1] + r * np.sin(phi)
        return np.stack((x, y, np.zeros_like(r)), axis=-1)

    @staticmethod
    def _radial_to_cartesian_3d(r: np.ndarray, theta: np.ndarray, phi: np.ndarray, center: GeometryMeshCard.Geometry.IdealMultilayerSphere.Center) -> np.ndarray:
        """将三维径向表面转换为最后一维按 x, y, z 排列的笛卡尔坐标"""
        if not isinstance(r, np.ndarray) or r.ndim < 2 or not isinstance(theta, np.ndarray) or theta.ndim != 1 or not isinstance(phi, np.ndarray) or phi.ndim != 1 or r.shape[-2:] != (len(theta), len(phi)):
            raise ValueError("三维径向表面的最后两维必须与一维极角和方位角数组长度相同")
        center_coordinates = np.array((center.x.value, center.y.value, center.z.value))
        if np.any(~np.isfinite(r)) or np.any(~np.isfinite(theta)) or np.any(~np.isfinite(phi)) or np.any(~np.isfinite(center_coordinates)):
            raise ValueError("三维径向表面, 极角, 方位角和中心坐标必须为有限数")
        sin_theta = np.sin(theta)[:, None]
        x = center_coordinates[0] + r * sin_theta * np.cos(phi)[None, :]
        y = center_coordinates[1] + r * sin_theta * np.sin(phi)[None, :]
        z = center_coordinates[2] + r * np.cos(theta)[:, None]
        return np.stack((x, y, z), axis=-1)

    @classmethod
    def _build_2d(cls, config: StochasticMultilayerParticleConfig, rng: np.random.Generator) -> GeometryTopology:
        """构造二维随机多层颗粒, Gmsh 生命周期由外部管理"""
        t_bar = cls._sample_mean_thicknesses(config, rng)
        a_outer, b_outer = cls._generate_outer_surface_2d(config, rng)
        a_layers, b_layers = cls._generate_layer_radial_profiles_2d(config, t_bar, a_outer, b_outer, rng)

        N = len(a_outer) - 1
        phi = np.linspace(0.0, 2 * np.pi, max(128, 8 * N), endpoint=False)
        radial_profiles = np.array([cls._evaluate_fourier_2d(a, b, phi)[0] for a, b in zip(a_layers, b_layers)])
        coordinates = cls._radial_to_cartesian_2d(radial_profiles, phi, config.center)

        curve_tags = []
        loop_tags = []
        for layer_coordinates in coordinates:
            point_tags = [gmsh.model.occ.addPoint(float(x), float(y), float(z)) for x, y, z in layer_coordinates]
            curve_tag = gmsh.model.occ.addSpline(point_tags + [point_tags[0]])
            curve_tags.append(curve_tag)
            loop_tags.append(gmsh.model.occ.addCurveLoop([curve_tag]))

        surface_tags = [gmsh.model.occ.addPlaneSurface([loop_tags[0]])]
        for l in range(1, len(loop_tags)):
            surface_tags.append(gmsh.model.occ.addPlaneSurface([loop_tags[l], loop_tags[l - 1]]))
        gmsh.model.occ.synchronize()

        regions = {layer.name: [surface_tags[l]] for l, layer in enumerate(config.layers.layer)}
        boundaries = {layer.outer_boundary.name: [curve_tags[l]] for l, layer in enumerate(config.layers.layer)}
        return GeometryTopology(dimension=2, regions=regions, boundaries=boundaries)

    @classmethod
    def _build_3d(cls, config: StochasticMultilayerParticleConfig, rng: np.random.Generator) -> GeometryTopology:
        """构造三维随机多层颗粒, Gmsh 生命周期由外部管理"""
        t_bar = cls._sample_mean_thicknesses(config, rng)
        a_nm_outer = cls._generate_outer_surface_3d(config, rng)
        a_nm_layers = cls._generate_layer_radial_profiles_3d(config, t_bar, a_nm_outer, rng)

        N = len(a_nm_outer) - 1
        theta_interval_count = max(16, N + 1)
        phi_count = max(32, 2 * N + 1)
        theta = np.linspace(0.0, np.pi, theta_interval_count + 1)
        phi = np.linspace(0.0, 2 * np.pi, phi_count, endpoint=False)
        radial_profiles = np.array([cls._evaluate_spherical_harmonics_3d(a_nm, theta, phi)[0] for a_nm in a_nm_layers])
        coordinates = cls._radial_to_cartesian_3d(radial_profiles, theta, phi, config.center)

        interface_surfaces = []
        shell_tags = []
        for layer_coordinates in coordinates:
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
            interface_surfaces.append(surface_tags)
            shell_tags.append(gmsh.model.occ.addSurfaceLoop(surface_tags))

        volume_tags = [gmsh.model.occ.addVolume([shell_tags[0]])]
        for l in range(1, len(shell_tags)):
            volume_tags.append(gmsh.model.occ.addVolume([shell_tags[l], shell_tags[l - 1]]))
        gmsh.model.occ.synchronize()

        regions = {layer.name: [volume_tags[l]] for l, layer in enumerate(config.layers.layer)}
        boundaries = {layer.outer_boundary.name: interface_surfaces[l] for l, layer in enumerate(config.layers.layer)}
        return GeometryTopology(dimension=3, regions=regions, boundaries=boundaries)

    @classmethod
    def _build_1d(cls, config: StochasticMultilayerParticleConfig, rng: np.random.Generator) -> GeometryTopology:
        """按抽样平均厚度构造径向线, Gmsh 生命周期由外部管理"""
        t_bar = cls._sample_mean_thicknesses(config, rng)
        r_bar = np.cumsum(t_bar)
        center_x = config.center.x.value
        points = [gmsh.model.occ.addPoint(center_x, 0.0, 0.0)]
        for r_l_bar in r_bar:
            points.append(gmsh.model.occ.addPoint(center_x + float(r_l_bar), 0.0, 0.0))
        regions = {}
        boundaries = {"particle_center": [points[0]]}
        for l, layer in enumerate(config.layers.layer):
            line = gmsh.model.occ.addLine(points[l], points[l + 1])
            regions[layer.name] = [line]
            boundaries[layer.outer_boundary.name] = [points[l + 1]]
        gmsh.model.occ.synchronize()
        return GeometryTopology(dimension=1, regions=regions, boundaries=boundaries)
