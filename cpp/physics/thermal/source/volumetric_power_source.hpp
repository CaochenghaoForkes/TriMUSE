#pragma once

#include <cstddef>

#include "properties/properties_field_data.hpp"

namespace trimuse
{

class VolumetricPowerSource
{
public:
    static double evaluate(const PropertiesFieldData& properties_field_data, std::size_t point_index);
};

}
