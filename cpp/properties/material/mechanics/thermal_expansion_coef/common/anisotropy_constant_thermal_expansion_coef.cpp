#include "properties/material/mechanics/thermal_expansion_coef/common/anisotropy_constant_thermal_expansion_coef.hpp"

namespace trimuse
{

AnisotropyConstantThermalExpansionCoef::AnisotropyConstantThermalExpansionCoef(const ModelParameterMap& parameters)
    :
    ConstantModel(VectorValue{std::get<double>(parameters.at("r_constant_value")), std::get<double>(parameters.at("t_constant_value"))}, "vector")
{
}

VectorValue AnisotropyConstantThermalExpansionCoef::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<VectorValue>(ConstantModel::evaluate(point));
}

}
