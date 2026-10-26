#include "physics/radioactivity/boundary/radioactivity_robin_boundary.hpp"

#include <variant>

namespace trimuse
{

RadioactivityRobinValues RadioactivityRobinBoundary::evaluate(const PropertiesFieldData& properties_field_data, int boundary_attribute)
{
    const double mass_transfer_coef = std::get<double>(properties_field_data.zone_values.at("derived.mass_transfer_coef").at(boundary_attribute).value());
    const double environment_concentration = std::get<double>(properties_field_data.zone_values.at("external.environment_nuclide_concentration").at(boundary_attribute).value());
    return {mass_transfer_coef, mass_transfer_coef * environment_concentration};
}

}
