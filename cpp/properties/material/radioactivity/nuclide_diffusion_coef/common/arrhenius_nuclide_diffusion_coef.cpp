#include "properties/material/radioactivity/nuclide_diffusion_coef/common/arrhenius_nuclide_diffusion_coef.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/common/arrhenius_terms.hpp"

namespace trimuse
{

ArrheniusNuclideDiffusionCoef::ArrheniusNuclideDiffusionCoef(const ModelParameterMap& parameters)
    : D0_(std::get<double>(parameters.at("D0"))),
      Q_(std::get<double>(parameters.at("Q")))
{
}

double ArrheniusNuclideDiffusionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    const double temperature = state_point.scalar("temperature"); // K
    return evaluate_arrhenius_term(D0_, Q_, temperature);
}

}
