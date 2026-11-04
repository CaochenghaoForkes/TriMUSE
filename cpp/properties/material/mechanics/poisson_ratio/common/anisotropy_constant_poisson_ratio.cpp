#include "properties/material/mechanics/poisson_ratio/common/anisotropy_constant_poisson_ratio.hpp"

namespace trimuse
{

AnisotropyConstantPoissonRatio::AnisotropyConstantPoissonRatio(const ModelParameterMap& parameters)
    : ConstantModel(VectorValue{std::get<double>(parameters.at("rt_constant_value")), std::get<double>(parameters.at("tt_constant_value"))}, "vector")
{
}

VectorValue AnisotropyConstantPoissonRatio::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<VectorValue>(ConstantModel::evaluate(point));
}

}
