#include "properties/material/radioactivity/nuclide_diffusion_coef/common/constant_nuclide_diffusion_coef.hpp"

namespace trimuse
{

ConstantNuclideDiffusionCoef::ConstantNuclideDiffusionCoef(const ModelParameterMap& parameters)
    : ConstantModel(parameters)
{
}

double ConstantNuclideDiffusionCoef::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(ConstantModel::evaluate(point));
}

}
