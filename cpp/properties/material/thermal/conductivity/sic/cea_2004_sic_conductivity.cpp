#include "properties/material/thermal/conductivity/sic/cea_2004_sic_conductivity.hpp"

namespace trimuse
{

Cea2004SicConductivity::Cea2004SicConductivity(const ModelParameterMap&)
{
}

double Cea2004SicConductivity::evaluate(const PropertiesStatePoint& state_point) const
{
    const double temperature = state_point.scalar("temperature");
    const double porosity = state_point.scalar("general.porosity");
    const double temperature_squared = temperature * temperature;
    const double temperature_cubed = temperature_squared * temperature;
    const double unirradiated_conductivity = 42.58 - 1.5564e4 / temperature + 1.2977e7 / temperature_squared - 1.8458e9 / temperature_cubed;
    const double irradiation_correction = 3.91112e-2 * std::exp(2.24732e-3 * temperature);
    const double porosity_correction = 1.0 - porosity;
    return checked_conductivity(unirradiated_conductivity * irradiation_correction * porosity_correction);
}

}
