#include "properties/derived/heat_transfer_coef/constant_heat_transfer_coef.hpp"

namespace trimuse
{

ConstantHeatTransferCoef::ConstantHeatTransferCoef(const ModelParameterMap& parameters)
    : ConstantModel(parameters)
{
}

StateValue ConstantHeatTransferCoef::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
