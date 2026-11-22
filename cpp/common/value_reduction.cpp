#include "common/value_reduction.hpp"

#include <algorithm>
#include <numeric>

namespace trimuse
{

double weighted_integral(const std::vector<double>& values, const std::vector<double>& weights) // 加权积分：I = Σᵢ wᵢvᵢ，其中 vᵢ 为第 i 个值，wᵢ 为对应的积分权重
{
    return std::inner_product(values.begin(), values.end(), weights.begin(), 0.0);
}

double weighted_average(const std::vector<double>& values, const std::vector<double>& weights) // 加权平均：v̄ = (Σᵢ wᵢvᵢ) / (Σᵢ wᵢ)
{
    return weighted_integral(values, weights) / std::accumulate(weights.begin(), weights.end(), 0.0);
}

double maximum(const std::vector<double>& values) // 最大值：vₘₐₓ = maxᵢ(vᵢ)
{
    return *std::max_element(values.begin(), values.end());
}

bool any_positive(const std::vector<double>& values) // 存在正值：若存在 i 使 vᵢ > 0，则返回 true，否则返回 false
{
    return std::any_of(values.begin(), values.end(), [](double value) { return value > 0.0; });
}

}
