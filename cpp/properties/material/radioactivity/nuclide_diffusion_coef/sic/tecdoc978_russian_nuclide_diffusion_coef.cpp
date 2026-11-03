#include "properties/material/radioactivity/nuclide_diffusion_coef/sic/tecdoc978_russian_nuclide_diffusion_coef.hpp"

#include <unordered_map>

namespace trimuse
{

TecDoc978RussianSiCNuclideDiffusionCoef::TecDoc978RussianSiCNuclideDiffusionCoef(const ModelParameterMap& parameters)
{
    static const std::unordered_map<std::string, ArrheniusTerms> coefficients{
        {"Cs", {{2.3e-17, 0.0}}}, // 表中仅给出 1000°C 的扩散系数，暂按常数使用。
        {"Ag", {{3.5e-10, 213e3}}},
    };
    terms_ = coefficients.at(std::get<std::string>(parameters.at("nuclide_symbol")));
}

double TecDoc978RussianSiCNuclideDiffusionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    return evaluate_arrhenius_terms(terms_, state_point.scalar("temperature"));
}

}
