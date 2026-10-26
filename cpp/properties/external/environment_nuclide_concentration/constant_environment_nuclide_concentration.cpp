#include "properties/external/environment_nuclide_concentration/constant_environment_nuclide_concentration.hpp"

namespace trimuse
{

ConstantEnvironmentNuclideConcentration::ConstantEnvironmentNuclideConcentration(const ModelParameterMap& parameters, const std::string& value_type)
    : ConstantModel(parameters, value_type)
{
}

StateValue ConstantEnvironmentNuclideConcentration::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
