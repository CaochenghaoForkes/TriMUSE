#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/state_value.hpp"

namespace trimuse
{

class PropertiesFieldData // 属性计算结果的存储容器：按属性作用的位置，分别保存全局、区域、边界分区和积分点上的属性值
{
public:
    using CachedValue = std::optional<StateValue>; // 一个可能尚未存入的属性值 有值的时候里面存着 StateValue 无值时是 std::nullopt

    using AttributeValues = std::unordered_map<int, CachedValue>; // 区域编号或边界分区编号 -> 属性值

    using IntegrationPointValues = std::vector<CachedValue>; // 一个属性在全域积分点上的值，连续存储 某积分点的下标为: fem_system.domain_quadrature_offset(element) + point_index 其中 element 是原网格单元编号，point_index 是该单元内的局部积分点编号

    std::unordered_map<std::string, CachedValue> global_values; // 属性名 -> 一个值

    std::unordered_map<std::string, AttributeValues> region_values; // 属性名 -> 区域编号 -> 一个值

    std::unordered_map<std::string, AttributeValues> zone_values; // 属性名 -> 边界分区编号 -> 一个值

    std::unordered_map<std::string, IntegrationPointValues> integration_point_values; // 属性名 -> 全域积分点数组
};

}
