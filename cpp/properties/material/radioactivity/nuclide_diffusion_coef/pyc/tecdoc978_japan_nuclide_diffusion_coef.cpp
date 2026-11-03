#include "properties/material/radioactivity/nuclide_diffusion_coef/pyc/tecdoc978_japan_nuclide_diffusion_coef.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/common/arrhenius_terms.hpp"

#include <stdexcept>

namespace trimuse
{

TecDoc978JapanPyCNuclideDiffusionCoef::TecDoc978JapanPyCNuclideDiffusionCoef(const ModelParameterMap& parameters)
{
    if (std::get<std::string>(parameters.at("nuclide_symbol")) != "Cs")
    {
        throw std::invalid_argument("TECDOC-978 日本 PyC 模型仅提供 Cs 系数");
    }
}

double TecDoc978JapanPyCNuclideDiffusionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    const double temperature = state_point.scalar("temperature"); // K
    if (temperature < 1600.0 + 273.15)
    {
        return evaluate_arrhenius_term(6.0e-9, 198e3, temperature);
    }
    return evaluate_arrhenius_term(1.2e-3, 412e3, temperature);
}

}
