#include "properties/material/mechanics/young_modulus/common/constant_young_modulus.hpp"

namespace trimuse
{

ConstantYoungModulus::ConstantYoungModulus(const ModelParameterMap& parameters)
    : ConstantModel(parameters)
{
}

double ConstantYoungModulus::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(ConstantModel::evaluate(point));
}

}
