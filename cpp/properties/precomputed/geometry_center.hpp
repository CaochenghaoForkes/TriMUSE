#pragma once

#include "properties/properties_system.hpp"

namespace trimuse
{

inline void initialize_geometry_center(PropertiesSystem& properties, const FemSystem& fem)
{
    const auto& center = fem.geometry_center();
    properties.set_precomputed("geometry_center", VectorValue(center.begin(), center.end()));
}

}
