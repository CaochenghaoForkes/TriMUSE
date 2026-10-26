#include "properties/external/nuclide_flux/constant_nuclide_flux.hpp"

namespace trimuse
{

ConstantNuclideFlux::ConstantNuclideFlux(const ModelParameterMap& parameters, const std::string& value_type)
    : ConstantModel(parameters, value_type)
{
}

StateValue ConstantNuclideFlux::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
