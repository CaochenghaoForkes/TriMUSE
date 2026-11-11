#pragma once

#include "fem/fem_system.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

void initialize_region_volume(PropertiesFieldData& properties, const FemSystem& fem);

}
