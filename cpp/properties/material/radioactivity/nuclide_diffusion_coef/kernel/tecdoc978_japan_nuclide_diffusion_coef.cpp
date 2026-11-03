#include "properties/material/radioactivity/nuclide_diffusion_coef/kernel/tecdoc978_japan_nuclide_diffusion_coef.hpp"

#include <unordered_map>

namespace trimuse
{

TecDoc978JapanUO2NuclideDiffusionCoef::TecDoc978JapanUO2NuclideDiffusionCoef(const ModelParameterMap& parameters)
{
    static const std::unordered_map<std::string, ArrheniusTerms> coefficients{
        {"Cs", {{4.7e-10, 177e3}}}
    };
    terms_ = coefficients.at(std::get<std::string>(parameters.at("nuclide_symbol")));
}

double TecDoc978JapanUO2NuclideDiffusionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    return evaluate_arrhenius_terms(terms_, state_point.scalar("temperature"));
}

}
