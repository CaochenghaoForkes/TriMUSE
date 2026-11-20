#include "common/value_reduction.hpp"

#include <algorithm>
#include <numeric>

namespace trimuse
{

double weighted_integral(const std::vector<double>& values, const std::vector<double>& weights)
{
    return std::inner_product(values.begin(), values.end(), weights.begin(), 0.0);
}

double weighted_average(const std::vector<double>& values, const std::vector<double>& weights)
{
    return weighted_integral(values, weights) / std::accumulate(weights.begin(), weights.end(), 0.0);
}

double maximum(const std::vector<double>& values)
{
    return *std::max_element(values.begin(), values.end());
}

bool any_positive(const std::vector<double>& values)
{
    return std::any_of(values.begin(), values.end(), [](double value) { return value > 0.0; });
}

}
