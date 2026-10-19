#include "physics/thermal/boundary/thermal_neumann_boundary.hpp"

#include <variant>

namespace trimuse
{

double ThermalNeumannBoundary::evaluate(const PropertiesFieldData& properties_field_data, int boundary_attribute)
{
    return -std::get<double>(properties_field_data.zone_values.at("heat_flux").at(boundary_attribute).value());
}

}
