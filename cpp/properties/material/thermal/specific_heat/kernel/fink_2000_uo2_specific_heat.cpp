#include "properties/material/thermal/specific_heat/kernel/fink_2000_uo2_specific_heat.hpp"

namespace trimuse
{

Fink2000Uo2SpecificHeat::Fink2000Uo2SpecificHeat(const ModelParameterMap& parameters)
{
    const double enrichment = std::get<double>(parameters.at("u235_enrichment"));
    molar_mass_ = 0.23504 * enrichment + 0.23805 * (1.0 - enrichment) + 0.03200;
}

double Fink2000Uo2SpecificHeat::evaluate(const PropertiesStatePoint& state_point) const
{
    const double reduced_temperature = state_point.scalar("temperature") / 1000.0;
    const double t2 = reduced_temperature * reduced_temperature;
    const double molar_specific_heat = 52.1743 + 87.951 * reduced_temperature - 84.2411 * t2
        + 31.542 * t2 * reduced_temperature - 2.6334 * t2 * t2 - 0.71391 / t2;
    return molar_specific_heat / molar_mass_;
}

}
