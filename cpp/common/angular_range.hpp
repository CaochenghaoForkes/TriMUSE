#pragma once

#include <array>
#include <cmath>
#include <optional>

#include "common/model_parameters.hpp"

namespace trimuse
{

inline bool is_in_angular_range(const std::array<double, 3>& position, const std::array<double, 3>& direction, double minimum, double maximum) // 判断一个点的方向是否落在指定角域里
{
    const auto [x, y, z] = position;
    const auto [dx, dy, dz] = direction;
    const double cross_norm = std::hypot(y * dz - z * dy, z * dx - x * dz, x * dy - y * dx); // ||r||*||d||*sin(a)
    const double angle = std::atan2(cross_norm, x * dx + y * dy + z * dz); // a = atan2(||r||*||d||*sin(a) / ||r||*||d||*cos(a))
    return minimum <= angle && angle <= maximum;
}

class AngularRanges
{
public:
    explicit AngularRanges(const ModelParameterMap& parameters); // 把输入卡里的角度参数整理成一组角域 每个角域在 ranges_ 里保存三项数据：夹角下限、夹角上限、中心方向
    bool empty() const; // 判断内部有没有保存角域 ranges_ 是存放角域的 std::vector
    std::optional<std::size_t> match(std::array<double, 3> position) const; // 接收一个相对颗粒中心的位置，返回第一个命中的角域下标 如果所有角域都没命中，就返回 std::nullopt
    static std::vector<double> number_array(const ModelParameterMap& parameters, const std::string& name); // 按参数名取值，把单个数和一组数统一成 std::vector<double>

private:
    struct Range // 角域 包含其相对中心的范围域张角
    {
        double minimum, maximum;
        std::array<double, 3> direction;
    };

    bool planar_; // 是否为 2D
    std::vector<Range> ranges_; // 存放角域的 std::vector
};

}
