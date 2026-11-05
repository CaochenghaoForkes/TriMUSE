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

// phi=0 时 Re(Y_nm)=L_nm。n+m 超过 170 时，库内部的 Gamma 比值可能在 double 中下溢；
// 这些高阶项改用更宽的浮点范围计算，避免 ARM 上 long double 与 double 范围相同造成模式丢失。
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
        profile.radius = std::get<double>(parameters.at("radius")); // 理想球或平底球接收该层半径
    }
    if (geometry_ == Geometry::Legendre || geometry_ == Geometry::SphericalHarmonics)
    {
        profile.coefficients = std::get<VectorValue>(parameters.at("coefficients"));
        profile.degree = geometry_ == Geometry::Legendre ? static_cast<int>(profile.coefficients.size()) - 1 : static_cast<int>(std::get<double>(parameters.at("degree"))); // 二维的系数是 a0 ... aN 共 N+1 项 所以要-1，三维的可以直接读 degree
    }
    if (geometry_ == Geometry::SphericalHarmonics)
    {
        const int N = profile.degree, width = 2 * N + 1;
        for (int n = 1; n <= N; ++n)
        for (int m = 1; m <= n; ++m)
        {   // 并非从 m=0 开始，是因为 an0 的实球谐基函数=关联 Legendre 函数，而其他分量上实球谐基函数 = \sqrt{2}*关联 Legendre 函数 * cos/sin
            profile.coefficients[n * width + N + m] *= std::sqrt(2.0); // 对应 coefficients[n, N + m] [n, N] 是第 n 阶的中点对应的位置
            profile.coefficients[n * width + N - m] *= std::sqrt(2.0); // 对应 coefficients[n, N - m]
        }
    }
    if (geometry_ == Geometry::Flat)
    {
        const double depth = std::get<double>(parameters.at("flattening_depth")); // 截平深度
        profile.fillet_radius = std::get<double>(parameters.at("fillet_radius")); // 圆角半径
        profile.distance = profile.radius - depth; // 中心到底面的距离 d = R - D
        profile.fillet_x = std::sqrt(depth * (2 * (profile.radius - profile.fillet_radius) - depth)); // 圆角圆心 x 坐标
        profile.fillet_y = profile.fillet_radius - profile.distance; // 圆角圆心 y 坐标
        profile.circle_angle = std::atan2(profile.fillet_x, profile.fillet_y); // 原球圆弧与圆角的连接处极角
        profile.flat_angle = std::atan2(profile.fillet_x, -profile.distance); // 圆球圆弧与底面相切连接处的极角
    }
    return profile;
}

MaterialCoordinateSystem::Sample MaterialCoordinateSystem::sample(const Profile& profile, double theta, double phi) const // 根据某个表面的 Profile 计算指定 (\theta,\phi) 的半径及角度导数返回 Sample（R，∂R/∂θ,(∂R/∂φ)/sinθ）
{
    if (profile.origin) 
    { 
        return {}; 
    }
    if (geometry_ == Geometry::Sphere) 
    { 
        return {profile.radius, 0.0, 0.0}; // 球面半径就是个常数与角度无关，因此两个导数均为 0
    }
    if (geometry_ == Geometry::SphericalHarmonics) 
    { 
        return spherical_sample(profile, theta, phi); // 计算三维随机颗粒的某个表面，在方向 (\theta,\phi) 上的半径和角度导数
    }
    const double sine = std::sin(theta), cosine = std::cos(theta);
    if (geometry_ == Geometry::Flat) 
    {
        if (theta <= profile.circle_angle) // 射线落在原始球圆弧上
        { 
            return {profile.radius, 0.0, 0.0}; // 半径恒定，两个角度导数为零
        }
        if (theta >= profile.flat_angle) // 射线落在底面 y = -d
        {
            return {-profile.distance / cosine, -profile.distance * sine / (cosine * cosine), 0.0};
        }
        const double projection = profile.fillet_x * sine + profile.fillet_y * cosine; // 圆心沿射线方向的投影
        const double perpendicular = profile.fillet_x * cosine - profile.fillet_y * sine; // 垂直于射线方向的单位方向是(cos\theta,sin\theta) 所以这一行是球心指向圆心的向量投影到垂直射线方向上
        const double root = std::sqrt(std::max(0.0, profile.fillet_radius * profile.fillet_radius - perpendicular * perpendicular)); // 圆心在射线上的垂足，到圆与射线交点的距离
        return {projection + root, perpendicular * (1.0 + projection / root), 0.0};
    }
    // Legendre 递推同时计算半径与 theta 导数，避免有限差分和步长选择。
    double previous = 0.0, value = 1.0, previous_derivative = 0.0, derivative = 0.0; //P0(cos) = 1 dP0/dtheta = 0
    Sample result;
    for (int n = 0; n <= profile.degree; ++n)
    {
        result.radius += profile.coefficients[n] * value;
        result.theta_derivative += profile.coefficients[n] * derivative;
        const double next = ((2 * n + 1) * cosine * value - n * previous) / (n + 1);
        const double next_derivative = ((2 * n + 1) * (-sine * value + cosine * derivative) - n * previous_derivative) / (n + 1);
        previous = value; value = next;
        previous_derivative = derivative; derivative = next_derivative;
    }
    return result;
}

MaterialCoordinateSystem::Sample MaterialCoordinateSystem::spherical_sample(const Profile& profile, double theta, double phi) const // 计算三维随机颗粒的某个表面，在方向 (\theta,\phi) 上的半径和角度导数
{
    const int N = profile.degree, width = 2 * N + 1;
    const double sine = std::sin(theta), cosine = std::cos(theta);
    Sample result;
    if (std::abs(sine) < 1e-12) // 在极点附近
    {
        // 两极只有 m=0 贡献半径、m=1 贡献一阶切向梯度；用解析极限避免除以 sin(theta)。
        const double sign = cosine >= 0.0 ? 1.0 : -1.0; // 区分两个极点 +1 为北级 -1 为南级
        double parity = 1.0, gradient_x = 0.0, gradient_y = 0.0; 
        for (int n = 0; n <= N; ++n) // 逐阶累加极点半径和切向梯度
        {
            result.radius += profile.coefficients[n * width + N] * std::sqrt((2 * n + 1) / (4 * pi)) * parity; // 累加极点半径
            if (n > 0) // 梯度从 1 阶开始有贡献
            {
                const double derivative = 0.5 * std::sqrt(n * (n + 1.0) * (2 * n + 1) / (4 * pi)) * parity * sign; // parity * sign = sigma^n * sigma = sigma^{n+1} // 第一次循环 parity = sigma^0 = 1
                gradient_x -= profile.coefficients[n * width + N + 1] * derivative;
                gradient_y -= profile.coefficients[n * width + N - 1] * derivative;
            }
            parity *= sign;
        }
        result.theta_derivative = sign * (gradient_x * std::cos(phi) + gradient_y * std::sin(phi));
        result.phi_derivative_over_sin = -gradient_x * std::sin(phi) + gradient_y * std::cos(phi);
        return result;
    }
    for (int m = 0; m <= N; ++m) // 普通位置：固定 m，依次遍历 n
    {
        double previous = 0.0; // 当前 m 下的 L_(n-1,m)，供解析导数使用
        const double cos_phi = std::cos(m * phi), sin_phi = std::sin(m * phi);
        for (int n = m; n <= N; ++n) // 固定 m 后，只遍历有效的 n = m,m+1,...,N
        {
            const double value = normalized_legendre(n, m, theta); // L_{nm}(\theta)
            const double coupling = n > m ? std::sqrt((2 * n + 1.0) * (n * n - m * m) / (2 * n - 1)) : 0.0;
            const double derivative = (n * cosine * value - coupling * previous) / sine;
            const double a = profile.coefficients[n * width + N + m];
            const double b = m > 0 ? profile.coefficients[n * width + N - m] : 0.0;
            const double angular = a * cos_phi + b * sin_phi;
            result.radius += value * angular;
            result.theta_derivative += derivative * angular;
            result.phi_derivative_over_sin += value * m * (-a * sin_phi + b * cos_phi) / sine;
            previous = value;
        }
    }
    return result;
}

Tensor3DValue MaterialCoordinateSystem::basis(int region_attribute, const std::array<double, 3>& position) const // 在指定区域的某个实际空间位置，计算材料厚度方向和两个切向方向，并将它们按列组成材料坐标变换矩阵 Q [[nx t1x t2x] [ny t1y t2y] [nz t1z t2z]] 二维对应计算坐标 (r,z,theta) 三维对应（x,y,z）
{
    Tensor3DValue result;
    for (int i = 0; i < 3; ++i) 
    { 
        result.data[i][i] = 1.0; // 初始化为单位阵
    }
    if (dimension_ == 1) 
    { 
        return result; // 球对称计算基底已经是材料径向与两个切向
    } 
    Direction relative{position[0] - center_[0], position[1] - center_[1], dimension_ == 3 ? position[2] - center_[2] : 0.0}; // 相对于颗粒几何中心的位置向量 x_{rel} = x - c 二维只使用前两个坐标，第三个置0
    const double radius = std::sqrt(relative[0] * relative[0] + relative[1] * relative[1] + relative[2] * relative[2]); // 该点到几何中心的距离
    if (radius == 0.0) 
    { 
        return result; // 中心方向无定义，取固定正交基底；各向同性材料不受影响
    } 
    const double theta = dimension_ == 2 ? std::atan2(relative[0], relative[1]) : std::atan2(std::hypot(relative[0], relative[1]), relative[2]); // 二维或三维的极角
    const double phi = dimension_ == 2 ? 0.0 : std::atan2(relative[1], relative[0]); // 三维的方向角 二维不需要
    const auto& profiles = profiles_.at(region_attribute); // 获取当前材料层的内外表面
    const Sample inner = sample(profiles[0], theta, phi), outer = sample(profiles[1], theta, phi); // 该方向内外表面的半径与角向导数
    const double s = (radius - inner.radius) / (outer.radius - inner.radius); // 该点的相对层间位置 s 于是可得到层内插值面 r = Rs(θ,φ) = (1-s)Rin(θ,φ) + sRout(θ,φ) 接下来要找的厚度方向，就是经过当前点的这个等 \(s\) 面的法向
    const double theta_slope = ((1.0 - s) * inner.theta_derivative + s * outer.theta_derivative) / radius;
    const double phi_slope = ((1.0 - s) * inner.phi_derivative_over_sin + s * outer.phi_derivative_over_sin) / radius;
    const double sine = std::sin(theta), cosine = std::cos(theta), cos_phi = std::cos(phi), sin_phi = std::sin(phi);
    const Direction radial = dimension_ == 2 ? Direction{sine, cosine, 0.0} : Direction{sine * cos_phi, sine * sin_phi, cosine}; // 二维计算坐标与三维计算坐标（笛卡尔坐标）er
    const Direction polar = dimension_ == 2 ? Direction{cosine, -sine, 0.0} : Direction{cosine * cos_phi, cosine * sin_phi, -sine}; // etheta
    const Direction azimuth{-sin_phi, cos_phi, 0.0}; // 三维方位角方向 ephi
    Direction normal;
    for (int i = 0; i < 3; ++i) 
    { 
        normal[i] = radial[i] - theta_slope * polar[i] - phi_slope * azimuth[i]; // 计算法向
    }
    normalize(normal); // 分别保存两个切向 t1 t2
    Direction tangent, second;
    if (dimension_ == 2) // n = (nr,nz,0)
    {
        tangent = {-normal[1], normal[0], 0.0}; second = {0.0, 0.0, 1.0};  // 取 t1=(-nz,nr,0) t2=(0,0,1)
    }
    else
    {
        int axis = 0;
        for (int i = 1; i < 3; ++i) 
        { 
            if (std::abs(normal[i]) < std::abs(normal[axis])) 
            { 
                axis = i; 
            } 
        }
        Direction auxiliary{}; 
        auxiliary[axis] = 1.0; // 选择与法向最不平行的坐标轴，避免叉乘退化；横观各向同性不要求唯一的切向
        tangent = cross(auxiliary, normal); 
        normalize(tangent);
        second = cross(normal, tangent);
    }
    for (int i = 0; i < 3; ++i)
    {
        result.data[i][0] = normal[i]; 
        result.data[i][1] = tangent[i]; 
        result.data[i][2] = second[i];
    }
    return result;
}

}
