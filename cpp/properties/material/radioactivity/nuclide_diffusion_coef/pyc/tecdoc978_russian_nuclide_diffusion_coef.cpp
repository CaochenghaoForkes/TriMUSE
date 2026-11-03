#include "properties/material/radioactivity/nuclide_diffusion_coef/pyc/tecdoc978_russian_nuclide_diffusion_coef.hpp"

#include <unordered_map>

namespace trimuse
{

TecDoc978RussianPyCNuclideDiffusionCoef::TecDoc978RussianPyCNuclideDiffusionCoef(const ModelParameterMap& parameters)
{
    static const std::unordered_map<std::string, ArrheniusTerms> coefficients{
        {"Ag", {{5.3e-4, 193e3}}},
        {"Kr", {{1.1e-5, 285e3}}},
    };
    terms_ = coefficients.at(std::get<std::string>(parameters.at("nuclide_symbol")));
}

double TecDoc978RussianPyCNuclideDiffusionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    return evaluate_arrhenius_terms(terms_, state_point.scalar("temperature"));
}

}
