#include "properties/material/thermal/specific_heat/sic/bison_2021_sic_specific_heat.hpp"

namespace trimuse
{

Bison2021SicSpecificHeat::Bison2021SicSpecificHeat(const ModelParameterMap&)
{
}

double Bison2021SicSpecificHeat::evaluate(const PropertiesStatePoint& state_point) const
{
    const double temperature = state_point.scalar("temperature");
    const double t2 = temperature * temperature;
    return 925.65 + 0.3772 * temperature - 7.9259e-5 * t2 - 3.1946e7 / t2;
}

}
