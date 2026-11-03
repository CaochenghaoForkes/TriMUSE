#include "properties/material/radioactivity/nuclide_diffusion_coef/sic/tecdoc978_japan_nuclide_diffusion_coef.hpp"

#include <unordered_map>

namespace trimuse
{

TecDoc978JapanSiCNuclideDiffusionCoef::TecDoc978JapanSiCNuclideDiffusionCoef(const ModelParameterMap& parameters)
{
    const std::string& symbol = std::get<std::string>(parameters.at("nuclide_symbol"));
    if (symbol == "Xe")
    {
        xenon_ = true;
        return;
    }
    if (symbol == "Cs")
    {
        caesium_ = true;
        return;
    }
    static const std::unordered_map<std::string, ArrheniusTerms> coefficients{
        {"Sr", {{1.2e-9, 205e3}}},
        {"Ag", {{6.8e-11, 177e3}}}
    };
    terms_ = coefficients.at(symbol);
}

double TecDoc978JapanSiCNuclideDiffusionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    const double temperature = state_point.scalar("temperature"); // K
    if (xenon_)
    {
        // Xe 的三组拟合在表中有重叠区间，约定在 1400°C 和 1650°C 切换
        if (temperature < 1400.0 + 273.15) 
        { 
            return evaluate_arrhenius_term(8.6e-10, 326e3, temperature); 
        }
        if (temperature < 1650.0 + 273.15) 
        { 
            return evaluate_arrhenius_term(3.7e1, 657e3, temperature); 
        }
        return evaluate_arrhenius_term(1.7, 624e3, temperature);
    }
    if (caesium_)
    {
        // Cs 的两组拟合之间有温度空档，约定在 1500°C 切换。
        if (temperature < 1500.0 + 273.15) 
        { 
            return evaluate_arrhenius_term(6.8e-12, 177e3, temperature); 
        }
        return evaluate_arrhenius_term(2.5e-2, 503e3, temperature);
    }
    return evaluate_arrhenius_terms(terms_, temperature);
}

}
