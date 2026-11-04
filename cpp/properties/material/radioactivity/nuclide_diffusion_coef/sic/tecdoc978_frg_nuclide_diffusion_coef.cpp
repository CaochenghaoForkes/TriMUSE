#include "properties/material/radioactivity/nuclide_diffusion_coef/sic/tecdoc978_frg_nuclide_diffusion_coef.hpp"

#include <cmath>
#include <unordered_map>

namespace trimuse
{

TecDoc978FRGSiCNuclideDiffusionCoef::TecDoc978FRGSiCNuclideDiffusionCoef(const ModelParameterMap& parameters)
{
    static const std::unordered_map<std::string, ArrheniusTerms> coefficients{
        {"Cs", {{5.5e-14, 125e3}, {1.6e-2, 514e3}}},
        {"Sr", {{1.2e-9, 205e3}, {1.8e6, 791e3}}},
        {"Ag", {{3.6e-9, 215e3}}},
    };
    const auto& symbol = std::get<std::string>(parameters.at("nuclide_symbol"));
    cesium_ = symbol == "Cs";
    terms_ = coefficients.at(symbol);
}

double TecDoc978FRGSiCNuclideDiffusionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    const double temperature = state_point.scalar("temperature");
    if (!cesium_)
    {
        return evaluate_arrhenius_terms(terms_, temperature);
    }

    // TECDOC-978 中 Γ 以 10^25 m^-2 为单位，Cs 第一项乘 exp(Γ/5)
    const double gamma = state_point.scalar("derived.fast_neutron_fluence") / 1e25;
    return evaluate_arrhenius_term(terms_[0].first * std::exp(gamma / 5.0), terms_[0].second, temperature)
         + evaluate_arrhenius_term(terms_[1].first, terms_[1].second, temperature);
}

}
