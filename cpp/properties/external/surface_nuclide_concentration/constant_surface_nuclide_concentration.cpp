#include "properties/external/surface_nuclide_concentration/constant_surface_nuclide_concentration.hpp"

namespace trimuse
{

ConstantSurfaceNuclideConcentration::ConstantSurfaceNuclideConcentration(const ModelParameterMap& parameters, const std::string& value_type)
    : ConstantModel(parameters, value_type)
{
}

StateValue ConstantSurfaceNuclideConcentration::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
