#include "properties/material/radioactivity/nuclide_diffusion_coef/sic/tecdoc978_usa_nuclide_diffusion_coef.hpp"

#include <stdexcept>
#include <unordered_map>

namespace trimuse
{

TecDoc978USASiCNuclideDiffusionCoef::TecDoc978USASiCNuclideDiffusionCoef(const ModelParameterMap& parameters)
{
    static const std::unordered_map<std::string, ArrheniusTerms> coefficients{
        {"Sr", {{1.2e-9, 205e3}, {1.8e6, 791e3}}},
        {"Ag", {{3.6e-9, 215e3}}},
    };
    terms_ = coefficients.at(std::get<std::string>(parameters.at("nuclide_symbol")));
}

double TecDoc978USASiCNuclideDiffusionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    return evaluate_arrhenius_terms(terms_, state_point.scalar("temperature"));
}

TecDoc978USALowerSiCNuclideDiffusionCoef::TecDoc978USALowerSiCNuclideDiffusionCoef(const ModelParameterMap& parameters)
{
    if (std::get<std::string>(parameters.at("nuclide_symbol")) != "Cs")
    {
        throw std::invalid_argument("TECDOC-978 美国 SiC lower 模型仅提供 Cs 系数");
    }
}

double TecDoc978USALowerSiCNuclideDiffusionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    static const ArrheniusTerms terms{{6.7e-14, 106e3}, {1.1e-4, 437e3}};
    const double temperature = state_point.scalar("temperature");
    return evaluate_arrhenius_terms(terms, temperature);
}

TecDoc978USAUpperSiCNuclideDiffusionCoef::TecDoc978USAUpperSiCNuclideDiffusionCoef(const ModelParameterMap& parameters)
{
    if (std::get<std::string>(parameters.at("nuclide_symbol")) != "Cs")
    {
        throw std::invalid_argument("TECDOC-978 美国 SiC upper 模型仅提供 Cs 系数");
    }
}

double TecDoc978USAUpperSiCNuclideDiffusionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    static const ArrheniusTerms terms{{6.7e-14, 106e3}, {2.4e-2, 482e3}};
    const double temperature = state_point.scalar("temperature");
    return evaluate_arrhenius_terms(terms, temperature);
}

}
