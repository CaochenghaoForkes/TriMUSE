#include "common/angular_range.hpp"

namespace trimuse
{

AngularRanges::AngularRanges(const ModelParameterMap& parameters) // 把输入卡里的角度参数整理成一组角域 每个角域在 ranges_ 里保存三项数据：夹角下限、夹角上限、中心方向
    :
    planar_(parameters.count("theta_mins") != 0) // 二维通过 theta_mins 识别
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

bool AngularRanges::empty() const // 判断内部有没有保存角域 ranges_ 是存放角域的 std::vector
{
    return ranges_.empty();
}

std::optional<std::size_t> AngularRanges::match(std::array<double, 3> position) const // 接收一个相对颗粒中心的位置，返回第一个命中的角域下标 如果所有角域都没命中，就返回 std::nullopt
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

std::vector<double> AngularRanges::number_array(const ModelParameterMap& parameters, const std::string& name) // 按参数名取值，把单个数和一组数统一成 std::vector<double>
{
    const auto& value = parameters.at(name);
    if (const auto* scalar = std::get_if<double>(&value))
    {
        return {*scalar};
    }
    return std::get<std::vector<double>>(value);
}

}
