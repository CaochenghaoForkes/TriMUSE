#include "physics/radioactivity/boundary/radioactivity_neumann_boundary.hpp"

#include <variant>

namespace trimuse
{

double RadioactivityNeumannBoundary::evaluate(const PropertiesFieldData& properties_field_data, int boundary_attribute)
{
    return -std::get<double>(properties_field_data.zone_values.at("external.nuclide_flux").at(boundary_attribute).value());
}

}
