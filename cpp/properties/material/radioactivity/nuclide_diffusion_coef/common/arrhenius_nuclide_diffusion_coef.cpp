#include "properties/material/radioactivity/nuclide_diffusion_coef/common/arrhenius_nuclide_diffusion_coef.hpp"

#include <cmath>

namespace trimuse
{

ArrheniusNuclideDiffusionCoef::ArrheniusNuclideDiffusionCoef(const ModelParameterMap& parameters)
    : D0_(std::get<double>(parameters.at("D0"))),
      Q_(std::get<double>(parameters.at("Q")))
{
}

double ArrheniusNuclideDiffusionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    constexpr double gas_constant = 8.31446261815324; // J/(mol·K)
    const double temperature = state_point.scalar("temperature"); // K
    return D0_ * std::exp(-Q_ / (gas_constant * temperature));
}

}
