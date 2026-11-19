#include "properties/external/energy_per_fission/constant_energy_per_fission.hpp"

namespace trimuse
{

ConstantEnergyPerFission::ConstantEnergyPerFission(const ModelParameterMap& parameters, const std::string& value_type)
    :
    ConstantModel(parameters, value_type)
{
}

StateValue ConstantEnergyPerFission::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
