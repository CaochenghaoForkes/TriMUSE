#include "properties/material/thermal/conductivity/common/constant_conductivity.hpp"

namespace trimuse
{

ConstantConductivity::ConstantConductivity(const ModelParameterMap& parameters)
    : ConstantModel(parameters)
{
}

double ConstantConductivity::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(ConstantModel::evaluate(point));
}

}
