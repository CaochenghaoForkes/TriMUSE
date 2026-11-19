#pragma once

#include "properties/properties_system.hpp"

namespace trimuse
{

inline void initialize_nuclide_symbol(PropertiesSystem& properties, const FemSystem&)
{
    const auto& name = std::get<std::string>(properties.field_data().global_values.at("precomputed.nuclide_name").value());
    const auto symbol = name.substr(0, name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"));
    properties.set_precomputed("nuclide_symbol", symbol);
}

}
