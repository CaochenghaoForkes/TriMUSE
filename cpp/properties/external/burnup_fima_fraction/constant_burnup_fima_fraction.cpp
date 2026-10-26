#include "properties/external/burnup_fima_fraction/constant_burnup_fima_fraction.hpp"

namespace trimuse
{

ConstantBurnupFimaFraction::ConstantBurnupFimaFraction(const ModelParameterMap& parameters, const std::string& value_type)
    : ConstantModel(parameters, value_type)
{
}

StateValue ConstantBurnupFimaFraction::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
