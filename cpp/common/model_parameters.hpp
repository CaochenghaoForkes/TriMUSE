#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace trimuse
{

using ModelParameterValue = std::variant<double, std::vector<double>>;
using ModelParameterMap = std::unordered_map<std::string, ModelParameterValue>;

}
