#pragma once

#include <cmath>
#include <utility>
#include <vector>

namespace trimuse
{

using ArrheniusTerms = std::vector<std::pair<double, double>>; // 每项为 (D0 [m²/s], Q [J/mol])

inline double evaluate_arrhenius_term(double D0, double Q, double temperature)
{
    constexpr double gas_constant = 8.31446261815324; // J/(mol·K)
    return D0 * std::exp(-Q / (gas_constant * temperature));
}

inline double evaluate_arrhenius_terms(const ArrheniusTerms& terms, double temperature)
{
    double diffusion = 0.0;
    for (const auto& [D0, Q] : terms)
    {
        diffusion += evaluate_arrhenius_term(D0, Q, temperature);
    }
    return diffusion;
}

}
