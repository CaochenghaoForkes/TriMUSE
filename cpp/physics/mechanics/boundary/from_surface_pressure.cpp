#include "physics/mechanics/boundary/from_surface_pressure.hpp"

namespace trimuse
{

MechanicsBoundaryValue MechanicsFromSurfacePressure::evaluate(const PropertiesFieldData& properties, int zone) const
{
    return std::get<double>(properties.zone_values.at("external.surface_pressure").at(zone).value());
}

MechanicsFromSurfacePressure::MechanicsFromSurfacePressure(const ModelParameterMap&)
{
}

}
