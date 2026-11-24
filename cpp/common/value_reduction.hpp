#pragma once

#include <vector>

namespace trimuse
{

double weighted_integral(const std::vector<double>& values, const std::vector<double>& weights); // Σ wi vi
double weighted_average(const std::vector<double>& values, const std::vector<double>& weights); // Σ wi vi / Σ wi
double maximum(const std::vector<double>& values); // 标量最大值
double minimum(const std::vector<double>& values); // 标量最小值
bool any_positive(const std::vector<double>& values); // 是否存在大于零的标量

}
