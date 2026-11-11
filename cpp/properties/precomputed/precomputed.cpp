#include "properties/precomputed/precomputed.hpp"

namespace trimuse
{

void initialize_precomputed(PropertiesFieldData& properties, const FemSystem& fem, const std::unordered_map<std::string, std::vector<std::string>>& field_required_properties)
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
