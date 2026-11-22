#include "properties/material/radioactivity/fission_gas_diffusion_coef/common/constant_fission_gas_diffusion_coef.hpp"

namespace trimuse
{

ConstantFissionGasDiffusionCoef::ConstantFissionGasDiffusionCoef(const ModelParameterMap& parameters)
    :
    ConstantModel(parameters)
{
}

double ConstantFissionGasDiffusionCoef::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(ConstantModel::evaluate(point));
}

}
