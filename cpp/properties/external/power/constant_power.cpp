#include "properties/external/power/constant_power.hpp"

namespace trimuse
{

ConstantPower::ConstantPower(const ModelParameterMap& parameters, const std::string& value_type)
    : ConstantModel(parameters, value_type)
{
}

StateValue ConstantPower::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
