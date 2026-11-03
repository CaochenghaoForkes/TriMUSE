#include "properties/derived/fast_neutron_fluence/constant_fast_neutron_fluence.hpp"

namespace trimuse
{

ConstantFastNeutronFluence::ConstantFastNeutronFluence(const ModelParameterMap& parameters)
    : ConstantModel(parameters)
{
}

StateValue ConstantFastNeutronFluence::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
