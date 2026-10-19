#pragma once

#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>

#include "common/state_value.hpp"

namespace trimuse
{

class PropertiesFieldData
{
public:
    using IntegrationPointKey = std::tuple<int, int, int>; // 单元编号, 求积阶次, 局部积分点编号

    using CachedValue = std::optional<StateValue>;

    using AttributeValues = std::unordered_map<int, CachedValue>;

    using IntegrationPointValues = std::map<IntegrationPointKey, CachedValue>;

    std::unordered_map<std::string, CachedValue> global_values;

    std::unordered_map<std::string, AttributeValues> region_values;

    std::unordered_map<std::string, AttributeValues> zone_values;

    std::unordered_map<std::string, IntegrationPointValues> integration_point_values;
};

}
