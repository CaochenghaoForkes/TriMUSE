#pragma once

#include <cmath>

#include "properties/properties_system.hpp"

namespace trimuse
{

inline void initialize_decay_constant(PropertiesSystem& properties, const FemSystem&)
{
    const double half_life = std::get<double>(properties.field_data().global_values.at("precomputed.half_life").value());
    properties.set_precomputed("decay_constant", std::log(2.0) / half_life);
}

}
