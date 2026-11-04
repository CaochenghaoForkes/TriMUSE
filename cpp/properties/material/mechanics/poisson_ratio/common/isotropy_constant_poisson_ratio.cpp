#include "properties/material/mechanics/poisson_ratio/common/isotropy_constant_poisson_ratio.hpp"

namespace trimuse
{

IsotropyConstantPoissonRatio::IsotropyConstantPoissonRatio(const ModelParameterMap& parameters)
    : ConstantModel(parameters.at("isotropy_constant_value"), "scalar")
{
}

VectorValue IsotropyConstantPoissonRatio::evaluate(const PropertiesStatePoint& point) const
{
    const double value = std::get<double>(ConstantModel::evaluate(point));
    return {value, value};
}

}
