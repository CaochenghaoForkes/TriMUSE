#include "properties/material/mechanics/thermal_expansion_coef/common/isotropy_constant_thermal_expansion_coef.hpp"

namespace trimuse
{

IsotropyConstantThermalExpansionCoef::IsotropyConstantThermalExpansionCoef(const ModelParameterMap& parameters)
    :
    ConstantModel(parameters.at("isotropy_constant_value"), "scalar")
{
}

VectorValue IsotropyConstantThermalExpansionCoef::evaluate(const PropertiesStatePoint& point) const
{
    const double value = std::get<double>(ConstantModel::evaluate(point));
    return {value, value};
}

}
