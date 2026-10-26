#include "properties/material/thermal/specific_heat/common/constant_specific_heat.hpp"

namespace trimuse
{

ConstantSpecificHeat::ConstantSpecificHeat(const ModelParameterMap& parameters)
    : ConstantModel(parameters)
{
}

double ConstantSpecificHeat::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(ConstantModel::evaluate(point));
}

}
