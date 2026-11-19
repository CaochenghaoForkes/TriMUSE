#include "physics/mechanics/boundary/from_surface_traction.hpp"

namespace trimuse
{

MechanicsBoundaryValue MechanicsFromSurfaceTraction::evaluate(const PropertiesFieldData& properties, int zone) const // 取得指定区域的面力
{
    return std::get<VectorValue>(properties.zone_values.at("external.surface_traction").at(zone).value());
}

MechanicsFromSurfaceTraction::MechanicsFromSurfaceTraction(const ModelParameterMap&)
{
}

}
