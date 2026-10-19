#include "physics/thermal/boundary/thermal_robin_boundary.hpp"

#include <variant>

namespace trimuse
{

RobinValues ThermalRobinBoundary::evaluate(const PropertiesFieldData& properties_field_data, int boundary_attribute)
{
    const double heat_transfer_coef = std::get<double>(properties_field_data.zone_values.at("heat_transfer_coef").at(boundary_attribute).value());
    const double environment_temperature = std::get<double>(properties_field_data.zone_values.at("environment_temperature").at(boundary_attribute).value());
    return {heat_transfer_coef, heat_transfer_coef * environment_temperature};
}

}
