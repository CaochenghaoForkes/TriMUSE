#pragma once

#include <array>
#include <string>
#include <unordered_map>

#include "common/model_parameters.hpp"
#include "common/state_value.hpp"

namespace trimuse
{

// 参数约定：球面为 radius；2D 随机轮廓为 Legendre coefficients；
// 3D 随机轮廓为 degree 和按 (N+1,2*N+1) 行优先展开的 coefficients；
// 平底轮廓为 radius、flattening_depth、fillet_radius
using RegionLayerGeometry = std::unordered_map<std::string, ModelParameterMap>; // inner、outer；空 inner 表示 kernel 中心
using RegionLayerGeometryMap = std::unordered_map<int, RegionLayerGeometry>; // 原网格区域编号 -> 内、外层实际构造参数

// 由内外轮廓的相对厚度坐标 s 构造材料正交基底，不依赖材料参数或求解场
class MaterialCoordinateSystem
{
public:
    MaterialCoordinateSystem(const std::string& geometry_method, int dimension, const std::array<double, 3>& center, const RegionLayerGeometryMap& region_layers);

    Tensor3DValue basis(int region_attribute, const std::array<double, 3>& position) const; // Q=[n,t1,t2]；2D 使用 (r,z,phi) 计算基底，3D 使用 (x,y,z)

private:
    enum class Geometry { Sphere, Legendre, SphericalHarmonics, Flat };

    struct Profile // 某一层表面的轮廓描述 每个区域有两个
    {
        bool origin = false; // 是否表示中心点 最内材料没有内表面 因此 origin = true
        double R0 = 0.0; // 理想球半径，或平底颗粒截平前的球半径 R_0
        VectorValue coefficients; // 2D 为 a_n；3D 为吸收 sqrt(2) 后的 A_nm、B_nm
        int N = 0; // 随机轮廓的最高展开阶数
        double d = 0.0, rho = 0.0, a = 0.0, b = 0.0; // 平底颗粒：中心到底面距离 d、圆角半径 rho、圆角圆心母线坐标 (a,b)
        double theta_c = 0.0; // 原球圆弧与圆角连接处的极角
        double theta_f = 0.0; // 圆角与底面相切连接处的极角
    };

    struct Sample // 某个表面轮廓在指定角度 (\theta,\phi) 处的求值结果 R dR/dtheta dR/(sin(theta)dphi)
    {
        double R = 0.0; // 该方向上的表面半径 R(theta,phi)
        double dR_dtheta = 0.0; // partial R / partial theta
        double dR_dphi_over_sin = 0.0; // (partial R / partial phi) / sin(theta)，两极使用解析极限
    };

    Profile make_profile(const ModelParameterMap& parameters) const;

    Sample sample(const Profile& profile, double theta, double phi) const; // 按轮廓类型调用对应方法

    Sample sphere_sample(const Profile& profile) const;

    Sample flat_sample(const Profile& profile, double theta) const;

    Sample stochastic_sample_2d(const Profile& profile, double theta) const;

    Sample stochastic_sample_3d(const Profile& profile, double theta, double phi) const;

    Geometry geometry_; // 构造几何方法

    int dimension_; // 维数

    std::array<double, 3> center_; // 中心坐标

    std::unordered_map<int, std::array<Profile, 2>> profiles_; // 区域编号 -> 内外侧的 Profile
};

}
