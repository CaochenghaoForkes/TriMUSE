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
    explicit AngularRanges(const ModelParameterMap& parameters) // 把输入卡里的角度参数整理成一组角域 每个角域在 ranges_ 里保存三项数据：夹角下限、夹角上限、中心方向
        : planar_(parameters.count("theta_mins") != 0) // 二维通过 theta_mins 识别
    {
        if (parameters.empty())
        {
            return;
        }
        const auto minimums = number_array(parameters, planar_ ? "theta_mins" : "angular_distance_mins");
        const auto maximums = number_array(parameters, planar_ ? "theta_maxs" : "angular_distance_maxs");
        const auto thetas = planar_ ? std::vector<double>{} : number_array(parameters, "thetas");
        const auto phis = planar_ ? std::vector<double>{} : number_array(parameters, "phis");
        for (std::size_t i = 0; i < minimums.size(); ++i)
        {
            std::array<double, 3> direction{0.0, 1.0, 0.0};
            if (!planar_)
            {
                const double theta = thetas[i], phi = phis[i];
                direction = {std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta)};
            }
            ranges_.push_back({minimums[i], maximums[i], direction});
        }
    }

    bool empty() const // 判断内部有没有保存角域 ranges_ 是存放角域的 std::vector
    { 
        return ranges_.empty(); 
    } 

    std::optional<std::size_t> match(std::array<double, 3> position) const // 接收一个相对颗粒中心的位置，返回第一个命中的角域下标 如果所有角域都没命中，就返回 std::nullopt
    {
        if (planar_)
        {
            position[2] = 0.0; // 2D只使用前两个分量
        }
        for (std::size_t i = 0; i < ranges_.size(); ++i)
        {
            const auto& range = ranges_[i];
            if (is_in_angular_range(position, range.direction, range.minimum, range.maximum))
            {
                return i; // 如果属于第 i 个角域就返回 i
            }
        }
        return std::nullopt; // 不属于任何角域
    }

    static std::vector<double> number_array(const ModelParameterMap& parameters, const std::string& name) // 按参数名取值，把单个数和一组数统一成 std::vector<double>
    {
        const auto& value = parameters.at(name);
        if (const auto* scalar = std::get_if<double>(&value))
        {
            return {*scalar};
        }
        return std::get<std::vector<double>>(value);
    }

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
