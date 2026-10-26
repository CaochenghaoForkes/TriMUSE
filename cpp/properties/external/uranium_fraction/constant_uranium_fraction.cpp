#include "properties/external/uranium_fraction/constant_uranium_fraction.hpp"

namespace trimuse
{

ConstantUraniumFraction::ConstantUraniumFraction(const ModelParameterMap& parameters, const std::string& value_type)
    : ConstantModel(parameters, value_type)
{
}

StateValue ConstantUraniumFraction::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
