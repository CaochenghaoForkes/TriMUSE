#include "properties/material/mechanics/young_modulus/common/isotropy_constant_young_modulus.hpp"

namespace trimuse
{

IsotropyConstantYoungModulus::IsotropyConstantYoungModulus(const ModelParameterMap& parameters)
    : ConstantModel(parameters.at("isotropy_constant_value"), "scalar")
{
}

VectorValue IsotropyConstantYoungModulus::evaluate(const PropertiesStatePoint& point) const
{
    const double value = std::get<double>(ConstantModel::evaluate(point));
    return {value, value};
}

}
