#pragma once

#include "fem/fem_system.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

inline void initialize_region_volume(PropertiesFieldData& properties, const FemSystem& fem)
{
    auto& values = properties.region_values.at("precomputed.region_volume");
    for (const auto& [region, volume] : fem.mesh_data().region_volumes())
    {
        values.at(region) = StateValue{volume};
    }
}

}
