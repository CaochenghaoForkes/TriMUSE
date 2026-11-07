#include "fem/material_coordinate_system.hpp"

#include <boost/math/special_functions/spherical_harmonic.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace trimuse
{
namespace
{
constexpr double pi = 3.14159265358979323846;
using Direction = std::array<double, 3>;

// phi=0 时 Re(Y_nm)=L_nmn+m 超过 170 时，库内部的 Gamma 比值可能在 double 中下溢；
// 这些高阶项改用更宽的浮点范围计算，避免 ARM 上 long double 与 double 范围相同造成模式丢失
double normalized_legendre(int n, int m, double theta)
{
    if (n + m <= 170)
    {
        return boost::math::spherical_harmonic_r(n, m, theta, 0.0);
    }
    using WideFloat = boost::multiprecision::cpp_bin_float_quad;
    return static_cast<double>(boost::math::spherical_harmonic_r(n, m, WideFloat(theta), WideFloat(0)));
}

Direction cross(const Direction& a, const Direction& b) // 计算叉积: 向量 a x b ；得到的返回值与它们垂直
{
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

void normalize(Direction& direction) // 将向量归一化为长度为 1 的单位向量
{
    const double length = std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
    for (double& value : direction)
    {
        value /= length;
    }
}
}

MaterialCoordinateSystem::MaterialCoordinateSystem(const std::string& geometry_method, int dimension, const std::array<double, 3>& center, const RegionLayerGeometryMap& region_layers) // 确定几何并保存各区域内外侧的 Profile
    : dimension_(dimension), center_(center)
{
    if (geometry_method == "ideal_multilayer_sphere")
    {
        geometry_ = Geometry::Sphere;
    }
    else if (geometry_method == "flat_multilayer_particle")
    {
        geometry_ = Geometry::Flat;
    }
    else if (geometry_method == "stochastic_multilayer_particle")
    {
        geometry_ = dimension == 1 ? Geometry::Sphere : dimension == 2 ? Geometry::Legendre : Geometry::SphericalHarmonics;
    }
    else
    {
        throw std::invalid_argument("尚未支持的材料方向几何: " + geometry_method);
    }
    for (const auto& [region, layers] : region_layers)
    {
        profiles_.emplace(region, std::array<Profile, 2>{make_profile(layers.at("inner")), make_profile(layers.at("outer"))});
    }
}

MaterialCoordinateSystem::Profile MaterialCoordinateSystem::make_profile(const ModelParameterMap& parameters) const // 根据 python 传入的字典制作一侧的 Profile
{
    Profile profile;
    profile.origin = parameters.empty(); // 最内材料的内层的 inner 是空字典，因此将其标记为 origin=true
    if (profile.origin)
    {
        return profile;
    }
    if (geometry_ == Geometry::Sphere || geometry_ == Geometry::Flat)
    {
        profile.R0 = std::get<double>(parameters.at("radius")); // 理想球或平底球接收该层半径
    }
    if (geometry_ == Geometry::Legendre || geometry_ == Geometry::SphericalHarmonics)
    {
        profile.coefficients = std::get<VectorValue>(parameters.at("coefficients"));
        profile.N = geometry_ == Geometry::Legendre ? static_cast<int>(profile.coefficients.size()) - 1 : static_cast<int>(std::get<double>(parameters.at("degree"))); // 二维的系数是 a0 ... aN 共 N+1 项 所以要-1，三维的可以直接读 degree
    }
    if (geometry_ == Geometry::SphericalHarmonics)
    {
        const int N = profile.N, width = 2 * N + 1;
        for (int n = 1; n <= N; ++n)
        for (int m = 1; m <= n; ++m)
        {   // m=0 的基函数为 L_n0；m>0 的实球谐基函数为 sqrt(2)*L_nm*cos/sin
            profile.coefficients[n * width + N + m] *= std::sqrt(2.0); // A_nm = sqrt(2)*a_nm^c；[n,N] 对应 m=0
            profile.coefficients[n * width + N - m] *= std::sqrt(2.0); // B_nm = sqrt(2)*a_nm^s
        }
    }
    if (geometry_ == Geometry::Flat)
    {
        const double D = std::get<double>(parameters.at("flattening_depth")); // 截平深度
        profile.rho = std::get<double>(parameters.at("fillet_radius")); // 圆角半径
        profile.d = profile.R0 - D; // 中心到底面的距离 d = R - D
        profile.a = std::sqrt(D * (2.0 * (profile.R0 - profile.rho) - D)); // a = sqrt(D[2(R_0-rho)-D])
        profile.b = profile.rho - profile.d; // b = rho - d = rho - R_0 + D
        profile.theta_c = std::atan2(profile.a, profile.b); // 原球圆弧与圆角的连接处极角
        profile.theta_f = std::atan2(profile.a, -profile.d); // 圆角与底面相切连接处的极角
    }
    return profile;
}

MaterialCoordinateSystem::Sample MaterialCoordinateSystem::sample(const Profile& profile, double theta, double phi) const // 按轮廓计算 R、partial R/partial theta、(partial R/partial phi)/sin(theta)
{
    if (profile.origin)
    {
        return {};
    }
    if (geometry_ == Geometry::Sphere)
    {
        return sphere_sample(profile);
    }
    if (geometry_ == Geometry::Flat)
    {
        return flat_sample(profile, theta);
    }
    if (geometry_ == Geometry::Legendre)
    {
        return stochastic_sample_2d(profile, theta);
    }
    return stochastic_sample_3d(profile, theta, phi);
}

MaterialCoordinateSystem::Sample MaterialCoordinateSystem::sphere_sample(const Profile& profile) const
{
    return {profile.R0, 0.0, 0.0}; // 球面 R=R_0，与两个角度无关
}

MaterialCoordinateSystem::Sample MaterialCoordinateSystem::flat_sample(const Profile& profile, double theta) const
{
    if (theta <= profile.theta_c) // 射线落在原球圆弧上
    {
        return sphere_sample(profile);
    }
    const double sin_theta = std::sin(theta), cos_theta = std::cos(theta);
    const double d = profile.d;
    if (theta >= profile.theta_f) // 射线落在底面 v=-d
    {
        return {-d / cos_theta, -d * sin_theta / (cos_theta * cos_theta), 0.0};
    }
    const double a = profile.a, b = profile.b, rho = profile.rho;
    const double p = a * sin_theta + b * cos_theta; // 圆角圆心沿射线方向的投影
    const double h = a * cos_theta - b * sin_theta; // 圆心到射线的有符号垂距，垂直方向为 (cos(theta),-sin(theta))
    const double ell = std::sqrt(std::max(0.0, rho * rho - h * h)); // 垂足到外侧交点的距离；保留浮点舍入保护
    const double R = p + ell;
    const double dR_dtheta = h * (1.0 + p / ell);
    return {R, dR_dtheta, 0.0};
}

MaterialCoordinateSystem::Sample MaterialCoordinateSystem::stochastic_sample_2d(const Profile& profile, double theta) const
{
    const double sin_theta = std::sin(theta), cos_theta = std::cos(theta);
    // p_n=P_n(cos(theta))，d_n=partial P_n(cos(theta))/partial theta
    // 与笔记一致，从 p_0=1、p_1=cos(theta)、d_0=0、d_1=-sin(theta) 开始
    double p_previous = 1.0, p_n = cos_theta;
    double d_previous = 0.0, d_n = -sin_theta;
    Sample surface{profile.coefficients[0], 0.0, 0.0}; // 先计入 a_0*p_0，a_0*d_0=0
    for (int n = 1; n <= profile.N; ++n)
    {
        const double a_n = profile.coefficients[n];
        surface.R += a_n * p_n;
        surface.dR_dtheta += a_n * d_n;
        const double p_next = ((2 * n + 1) * cos_theta * p_n - n * p_previous) / (n + 1);
        const double d_next = ((2 * n + 1) * (-sin_theta * p_n + cos_theta * d_n) - n * d_previous) / (n + 1);
        p_previous = p_n; p_n = p_next;
        d_previous = d_n; d_n = d_next;
    }
    return surface;
}

MaterialCoordinateSystem::Sample MaterialCoordinateSystem::stochastic_sample_3d(const Profile& profile, double theta, double phi) const // 计算三维随机轮廓 R(theta,phi) 及两个角向导数
{
    const int N = profile.N, width = 2 * N + 1;
    const double sin_theta = std::sin(theta), cos_theta = std::cos(theta);
    Sample surface;
    if (std::abs(sin_theta) < 1e-12) // 两极附近使用解析极限，避免除以 sin(theta)
    {
        // 两极只有 m=0 贡献半径、m=1 贡献一阶切向梯度
        const double sigma = cos_theta >= 0.0 ? 1.0 : -1.0; // 北极 +1，南极 -1
        double sigma_n = 1.0, g_x = 0.0, g_y = 0.0; // 循环开始时 sigma_n=sigma^n，从 n=0 开始
        for (int n = 0; n <= N; ++n)
        {
            const double A_n0 = profile.coefficients[n * width + N];
            surface.R += A_n0 * std::sqrt((2 * n + 1) / (4 * pi)) * sigma_n;
            if (n > 0)
            {
                const double d_n = 0.5 * std::sqrt(n * (n + 1.0) * (2 * n + 1) / (4 * pi)) * sigma_n * sigma;
                const double A_n1 = profile.coefficients[n * width + N + 1];
                const double B_n1 = profile.coefficients[n * width + N - 1];
                g_x -= A_n1 * d_n;
                g_y -= B_n1 * d_n;
            }
            sigma_n *= sigma;
        }
        // 将极点的笛卡尔切向梯度投影到 e_theta、e_phi
        surface.dR_dtheta = sigma * (g_x * std::cos(phi) + g_y * std::sin(phi));
        surface.dR_dphi_over_sin = -g_x * std::sin(phi) + g_y * std::cos(phi);
        return surface;
    }
    VectorValue L_previous(N + 1, 0.0); // L_previous[m] 保存当前阶所需的 L_(n-1,m)
    for (int n = 0; n <= N; ++n) // 按笔记顺序：先遍历 n，再遍历 m=0,...,n
    {
        for (int m = 0; m <= n; ++m)
        {
            const double cos_m_phi = std::cos(m * phi), sin_m_phi = std::sin(m * phi);
            const double L_nm = normalized_legendre(n, m, theta);
            const double c_nm = n > m ? std::sqrt((2 * n + 1.0) * (n * n - m * m) / (2 * n - 1)) : 0.0;
            const double dL_dtheta = (n * cos_theta * L_nm - c_nm * L_previous[m]) / sin_theta;
            const double A_nm = profile.coefficients[n * width + N + m];
            const double B_nm = m > 0 ? profile.coefficients[n * width + N - m] : 0.0;
            const double H_nm = A_nm * cos_m_phi + B_nm * sin_m_phi;
            const double dR_dphi = L_nm * m * (-A_nm * sin_m_phi + B_nm * cos_m_phi);
            surface.R += L_nm * H_nm;
            surface.dR_dtheta += dL_dtheta * H_nm;
            surface.dR_dphi_over_sin += dR_dphi / sin_theta;
            L_previous[m] = L_nm;
        }
    }
    return surface;
}

Tensor3DValue MaterialCoordinateSystem::basis(int region_attribute, const std::array<double, 3>& position) const // 由当前积分点的等 s 面法向建立 Q=[n,t1,t2]
{
    Tensor3DValue Q;
    for (int i = 0; i < 3; ++i)
    {
        Q.data[i][i] = 1.0;
    }
    if (dimension_ == 1)
    {
        return Q; // 球对称计算基底已经是材料径向与两个切向
    }
    const Direction relative{position[0] - center_[0], position[1] - center_[1], dimension_ == 3 ? position[2] - center_[2] : 0.0}; // x_relative=x-c；2D 只使用前两个坐标
    const double r = std::sqrt(relative[0] * relative[0] + relative[1] * relative[1] + relative[2] * relative[2]);
    if (r == 0.0)
    {
        return Q; // 中心方向无定义，取固定正交基底；各向同性材料不受影响
    }
    const double theta = dimension_ == 2 ? std::atan2(relative[0], relative[1]) : std::atan2(std::hypot(relative[0], relative[1]), relative[2]);
    const double phi = dimension_ == 2 ? 0.0 : std::atan2(relative[1], relative[0]);
    const auto& profiles = profiles_.at(region_attribute);
    const Sample inner = sample(profiles[0], theta, phi), outer = sample(profiles[1], theta, phi);

    // s_0=(r-R_in)/(R_out-R_in)；固定 s_0 后，R_s0=(1-s_0)R_in+s_0*R_out
    const double s0 = (r - inner.R) / (outer.R - inner.R);
    const double dR_s0_dtheta = (1.0 - s0) * inner.dR_dtheta + s0 * outer.dR_dtheta;
    const double dR_s0_dphi_over_sin = (1.0 - s0) * inner.dR_dphi_over_sin + s0 * outer.dR_dphi_over_sin;

    const double sin_theta = std::sin(theta), cos_theta = std::cos(theta), cos_phi = std::cos(phi), sin_phi = std::sin(phi);
    const Direction e_r = dimension_ == 2 ? Direction{sin_theta, cos_theta, 0.0} : Direction{sin_theta * cos_phi, sin_theta * sin_phi, cos_theta};
    const Direction e_theta = dimension_ == 2 ? Direction{cos_theta, -sin_theta, 0.0} : Direction{cos_theta * cos_phi, cos_theta * sin_phi, -sin_theta};
    const Direction e_phi{-sin_phi, cos_phi, 0.0}; // 2D 的方位角导数为零，此项不参与法向
    Direction grad_F;
    for (int i = 0; i < 3; ++i)
    {
        grad_F[i] = e_r[i] - dR_s0_dtheta / r * e_theta[i] - dR_s0_dphi_over_sin / r * e_phi[i];
    }
    Direction n = grad_F;
    normalize(n); // n=grad(F)/|grad(F)|
    Direction t1, t2;
    if (dimension_ == 2) // 计算基底为 (r,z,phi)，n=(n_r,n_z,0)
    {
        t1 = {-n[1], n[0], 0.0};
        t2 = {0.0, 0.0, 1.0};
    }
    else
    {
        int axis = 0;
        for (int i = 1; i < 3; ++i)
        {
            if (std::abs(n[i]) < std::abs(n[axis]))
            {
                axis = i;
            }
        }
        Direction auxiliary{};
        auxiliary[axis] = 1.0; // 选择与 n 最不平行的坐标轴；横观各向同性不要求唯一的切向
        t1 = cross(auxiliary, n);
        normalize(t1);
        t2 = cross(n, t1);
    }
    for (int i = 0; i < 3; ++i)
    {
        Q.data[i][0] = n[i];
        Q.data[i][1] = t1[i];
        Q.data[i][2] = t2[i];
    }
    return Q;
}

}
