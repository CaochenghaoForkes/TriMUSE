#include "properties/material/general/porosity/common/constant_porosity.hpp"

namespace trimuse
{

ConstantPorosity::ConstantPorosity(const ModelParameterMap& parameters)
    : ConstantModel(parameters)
{
}

double ConstantPorosity::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(ConstantModel::evaluate(point));
}

}
