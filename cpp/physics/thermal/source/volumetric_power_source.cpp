#include "physics/thermal/source/volumetric_power_source.hpp"

#include <variant>

namespace trimuse
{

double VolumetricPowerSource::evaluate(const PropertiesFieldData& properties_field_data, std::size_t point_index)
{
    return std::get<double>(properties_field_data.integration_point_values.at("volumetric_power")[point_index].value()); // Q 的单位为 W/m³
}

}
