#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "properties/precomputed/region_volume.hpp"
#include "properties/precomputed/tensor_transform.hpp"

namespace trimuse
{

// field_required_properties 只包含本次启用的场；固定几何数据在构造时填入一次
inline void initialize_precomputed(PropertiesFieldData& properties, const FemSystem& fem, const std::unordered_map<std::string, std::vector<std::string>>& field_required_properties)
{
    initialize_region_volume(properties, fem);
    for (const auto& [field, required] : field_required_properties)
    {
        if (std::find(required.begin(), required.end(), "material.mechanics.elasticity_tensor") != required.end())
        {
            initialize_tensor_transform(properties, fem);
            return;
        }
    }
}

}
