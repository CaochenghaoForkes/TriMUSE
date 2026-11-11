#pragma once

#include <cmath>
#include <utility>
#include <vector>

namespace trimuse
{

using ArrheniusTerms = std::vector<std::pair<double, double>>; // 每项为 (D0 [m²/s], Q [J/mol])

double evaluate_arrhenius_term(double D0, double Q, double temperature);

double evaluate_arrhenius_terms(const ArrheniusTerms& terms, double temperature);

}
