#include "properties/material/radioactivity/nuclide_diffusion_coef/pyc/tecdoc978_usa_nuclide_diffusion_coef.hpp"

#include <unordered_map>

namespace trimuse
{

TecDoc978USAPyCNuclideDiffusionCoef::TecDoc978USAPyCNuclideDiffusionCoef(const ModelParameterMap& parameters)
{
    static const std::unordered_map<std::string, ArrheniusTerms> coefficients{
        {"Cs", {{5.0e-5, 318e3}}},
        {"Sr", {{2.3e-6, 197e3}}},
        {"Ag", {{5.3e-9, 154e3}}},
        {"Kr", {{2.9e-8, 291e3}, {2.0e5, 923e3}}},
        {"Xe", {{2.9e-8, 291e3}, {2.0e5, 923e3}}},
    };
    terms_ = coefficients.at(std::get<std::string>(parameters.at("nuclide_symbol")));
}

double TecDoc978USAPyCNuclideDiffusionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    return evaluate_arrhenius_terms(terms_, state_point.scalar("temperature"));
}

}
