#include "properties/external/fast_neutron_flux/constant_fast_neutron_flux.hpp"

namespace trimuse
{

ConstantFastNeutronFlux::ConstantFastNeutronFlux(const ModelParameterMap& parameters, const std::string& value_type)
    : ConstantModel(parameters, value_type)
{
}

StateValue ConstantFastNeutronFlux::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
