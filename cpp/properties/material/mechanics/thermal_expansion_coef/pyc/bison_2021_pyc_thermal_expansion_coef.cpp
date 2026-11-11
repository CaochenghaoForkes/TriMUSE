#include "properties/material/mechanics/thermal_expansion_coef/pyc/bison_2021_pyc_thermal_expansion_coef.hpp"

namespace trimuse
{

Bison2021PycThermalExpansionCoef::Bison2021PycThermalExpansionCoef(const ModelParameterMap&)
{
}

// Jiang et al., Journal of Nuclear Materials 548 (2021) 152795，式（20–22）
VectorValue Bison2021PycThermalExpansionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    const double temperature = state_point.scalar("temperature");
    const double baf = state_point.scalar("material.general.bacon_anisotropy_factor");
    const double radial_orientation = 2.0 / (2.0 + baf);
    const double tangential_orientation = (1.0 + baf) / (2.0 + baf);
    const double temperature_factor = 1e-6 * (1.0 + 0.11 * (temperature - 673.0) / 700.0);
    const double tangential_difference = tangential_orientation - 1.0;
    return {(30.0 - 37.5 * radial_orientation) * temperature_factor, (36.0 * tangential_difference * tangential_difference + 1.0) * temperature_factor};
}

}
