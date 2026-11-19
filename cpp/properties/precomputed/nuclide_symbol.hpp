#pragma once

#include "properties/properties_system.hpp"

namespace trimuse
{

inline void initialize_nuclide_symbol(PropertiesSystem& properties, const ModelParameterMap& parameters)
{
    if (const auto entry = parameters.find("nuclide_name"); entry != parameters.end())
    {
        const auto& name = std::get<std::string>(entry->second);
        const auto symbol = name.substr(0, name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"));
        properties.set_precomputed("nuclide_symbol", symbol);
    }
}

}
