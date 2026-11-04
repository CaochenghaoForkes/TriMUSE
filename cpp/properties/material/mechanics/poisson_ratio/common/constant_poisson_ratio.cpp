#include "properties/material/mechanics/poisson_ratio/common/constant_poisson_ratio.hpp"

namespace trimuse
{

ConstantPoissonRatio::ConstantPoissonRatio(const ModelParameterMap& parameters)
    : ConstantModel(parameters)
{
}

double ConstantPoissonRatio::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(ConstantModel::evaluate(point));
}

}
