#include "properties/material/radioactivity/nuclide_diffusion_coef/kernel/tecdoc978_frg_nuclide_diffusion_coef.hpp"

#include <unordered_map>

namespace trimuse
{

TecDoc978FRGUO2NuclideDiffusionCoef::TecDoc978FRGUO2NuclideDiffusionCoef(const ModelParameterMap& parameters)
{
    static const std::unordered_map<std::string, ArrheniusTerms> coefficients{
        {"Cs", {{5.6e-8, 209e3}, {5.2e-4, 362e3}}},
        {"Sr", {{2.2e-3, 488e3}}},
        {"Ag", {{6.7e-9, 165e3}}},
        {"I",  {{1.3e-12, 126e3}}},
        {"Kr", {{1.3e-12, 126e3}}},
        {"Xe", {{1.3e-12, 126e3}}},
    };
    terms_ = coefficients.at(std::get<std::string>(parameters.at("nuclide_symbol")));
}

double TecDoc978FRGUO2NuclideDiffusionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    return evaluate_arrhenius_terms(terms_, state_point.scalar("temperature"));
}

}
