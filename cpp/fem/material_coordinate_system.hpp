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
// 平底轮廓为 radius、flattening_depth、fillet_radius。
using RegionLayerGeometry = std::unordered_map<std::string, ModelParameterMap>; // inner、outer；空 inner 表示 kernel 中心
using RegionLayerGeometryMap = std::unordered_map<int, RegionLayerGeometry>; // 原网格区域编号 -> 内、外层实际构造参数

// 由内外轮廓的相对厚度坐标 s 构造材料正交基底，不依赖材料参数或求解场
class MaterialCoordinateSystem
{
public:
    MaterialCoordinateSystem(const std::string& geometry_method, int dimension, const std::array<double, 3>& center, const RegionLayerGeometryMap& region_layers);

    Tensor3DValue basis(int region_attribute, const std::array<double, 3>& position) const; // Q 的列为厚度方向和两个切向；2D 使用 (r,z,theta) 基底，3D 使用 (x,y,z)

private:
    enum class Geometry { Sphere, Legendre, SphericalHarmonics, Flat };

    struct Profile // 某一层表面的轮廓描述 每个区域有两个
    {
        bool origin = false; // 是否表示中心点 最内材料没有内表面 因此 origin = true
        double radius = 0.0; // 理想球的半径
        VectorValue coefficients; // 2D 为 Legendre 系数 3D 为实球谐函数
        int degree = 0; // 随机轮廓的最高展开阶次
        double distance = 0.0, fillet_radius = 0.0, fillet_x = 0.0, fillet_y = 0.0; // 平底颗粒中心到底部平面的距离 平底颗粒的圆角半径 圆角圆心在母线平面中的横坐标 圆角圆心在母线平面中的轴向坐标
        double circle_angle = 0.0; // 原球圆弧与圆角的连接处极角
        double flat_angle = 0.0; // 圆球圆弧与底面相切连接处的极角
    };

    struct Sample // 某个表面轮廓在指定角度 (\theta,\phi) 处的求值结果 R dR/dtheta dR/(sin(theta)dphi)
    {
        double radius = 0.0; // 该方向上的表面半径 R(\theta,\phi)
        double theta_derivative = 0.0; // (\partial R) / (\partial \theta)
        double phi_derivative_over_sin = 0.0; // (\partial R) / (\partial \phi * sin( \theta ))
    };

    Profile make_profile(const ModelParameterMap& parameters) const;

    Sample sample(const Profile& profile, double theta, double phi) const;

    Sample spherical_sample(const Profile& profile, double theta, double phi) const;

    Geometry geometry_; // 构造几何方法

    int dimension_; // 维数

    std::array<double, 3> center_; // 中心坐标

    std::unordered_map<int, std::array<Profile, 2>> profiles_; // 区域编号 -> 内外侧的 Profile
};

}
