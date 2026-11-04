#include "properties/material/mechanics/shear_modulus/common/constant_shear_modulus.hpp"

namespace trimuse
{

ConstantShearModulus::ConstantShearModulus(const ModelParameterMap& parameters)
    : ConstantModel(parameters)
{
}

double ConstantShearModulus::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(ConstantModel::evaluate(point));
}

}
