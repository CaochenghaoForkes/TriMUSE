#include "properties/material/radioactivity/nuclide_diffusion_coef/sic/tecdoc978_frg_nuclide_diffusion_coef.hpp"

#include <cmath>
#include <unordered_map>

namespace trimuse
{

TecDoc978FRGSiCNuclideDiffusionCoef::TecDoc978FRGSiCNuclideDiffusionCoef(const ModelParameterMap& parameters)
{
    // 暂以 Γ=1 代替快中子注量，对应 10^25 m^-2；表中说明 Γ 与反应堆有关。
    constexpr double normalized_fast_fluence = 1.0;
    static const std::unordered_map<std::string, ArrheniusTerms> coefficients{
        {"Cs", {{5.5e-14 * std::exp(normalized_fast_fluence / 5.0), 125e3}, {1.6e-2, 514e3}}},
        {"Sr", {{1.2e-9, 205e3}, {1.8e6, 791e3}}},
        {"Ag", {{3.6e-9, 215e3}}},
    };
    terms_ = coefficients.at(std::get<std::string>(parameters.at("nuclide_symbol")));
}

double TecDoc978FRGSiCNuclideDiffusionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    return evaluate_arrhenius_terms(terms_, state_point.scalar("temperature"));
}

}
