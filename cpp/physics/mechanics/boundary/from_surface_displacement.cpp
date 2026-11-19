#include "physics/mechanics/boundary/from_surface_displacement.hpp"

namespace trimuse
{

MechanicsBoundaryValue MechanicsFromSurfaceDisplacement::evaluate(const PropertiesFieldData& properties, int zone) const
{
    return std::get<VectorValue>(properties.zone_values.at("external.surface_displacement").at(zone).value());
}

MechanicsFromSurfaceDisplacement::MechanicsFromSurfaceDisplacement(const ModelParameterMap&)
{
}

}
