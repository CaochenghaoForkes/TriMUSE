#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace trimuse
{

using ModelParameterValue = std::variant<double, std::vector<double>, std::string>; // 模型参数值: 一个数、一组数或配置用的名称
using ModelParameterMap = std::unordered_map<std::string, ModelParameterValue>; // 模型参数字典 参数名称 -> 参数值

}
