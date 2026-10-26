#include "properties/derived/mass_transfer_coef/constant_mass_transfer_coef.hpp"

namespace trimuse
{

ConstantMassTransferCoef::ConstantMassTransferCoef(const ModelParameterMap& parameters)
    : ConstantModel(parameters)
{
}

StateValue ConstantMassTransferCoef::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
