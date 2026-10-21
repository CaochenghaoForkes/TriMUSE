#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/state_value.hpp"

namespace trimuse
{

class PropertiesFieldData
{
public:
    using CachedValue = std::optional<StateValue>;

    using AttributeValues = std::unordered_map<int, CachedValue>;

    using IntegrationPointValues = std::vector<CachedValue>; // 下标=FemSystem 的单元偏移+局部积分点编号, 未安排的位置为空

    std::unordered_map<std::string, CachedValue> global_values;

    std::unordered_map<std::string, AttributeValues> region_values;

    std::unordered_map<std::string, AttributeValues> zone_values;

    std::unordered_map<std::string, IntegrationPointValues> integration_point_values;
};

}
