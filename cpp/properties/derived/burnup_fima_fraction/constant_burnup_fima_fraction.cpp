#include "properties/derived/burnup_fima_fraction/constant_burnup_fima_fraction.hpp"

namespace trimuse
{

ConstantBurnupFimaFraction::ConstantBurnupFimaFraction(const ModelParameterMap& parameters)
    :
    ConstantModel(parameters)
{
}

StateValue ConstantBurnupFimaFraction::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
