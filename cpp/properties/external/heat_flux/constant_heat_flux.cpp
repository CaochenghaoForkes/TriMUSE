#include "properties/external/heat_flux/constant_heat_flux.hpp"

namespace trimuse
{

ConstantHeatFlux::ConstantHeatFlux(const ModelParameterMap& parameters, const std::string& value_type)
    : ConstantModel(parameters, value_type)
{
}

StateValue ConstantHeatFlux::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
