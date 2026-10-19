#include "physics/thermal/boundary/thermal_dirichlet_boundary.hpp"

#include <variant>

namespace trimuse
{

double ThermalDirichletBoundary::evaluate(const PropertiesFieldData& properties_field_data, int boundary_attribute)
{
    return std::get<double>(properties_field_data.zone_values.at("surface_temperature").at(boundary_attribute).value());
}

}
