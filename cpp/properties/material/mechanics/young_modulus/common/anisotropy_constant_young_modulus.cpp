#include "properties/material/mechanics/young_modulus/common/anisotropy_constant_young_modulus.hpp"

namespace trimuse
{

AnisotropyConstantYoungModulus::AnisotropyConstantYoungModulus(const ModelParameterMap& parameters)
    : ConstantModel(VectorValue{std::get<double>(parameters.at("r_constant_value")), std::get<double>(parameters.at("t_constant_value"))}, "vector")
{
}

VectorValue AnisotropyConstantYoungModulus::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<VectorValue>(ConstantModel::evaluate(point));
}

}
