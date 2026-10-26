#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace trimuse
{

using ModelParameterValue = std::variant<double, std::vector<double>>; // 模型的单个参数值的类型可以是一个 double 或 std::vector<double>>
using ModelParameterMap = std::unordered_map<std::string, ModelParameterValue>; // 模型参数字典 参数名称 -> 一个数或一组数

}
