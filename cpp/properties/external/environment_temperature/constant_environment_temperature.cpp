#include "properties/external/environment_temperature/constant_environment_temperature.hpp"

namespace trimuse
{

ConstantEnvironmentTemperature::ConstantEnvironmentTemperature(const ModelParameterMap& parameters, const std::string& value_type)
    : ConstantModel(parameters, value_type)
{
}

StateValue ConstantEnvironmentTemperature::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
