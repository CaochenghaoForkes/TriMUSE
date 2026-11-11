#include "properties/material/mechanics/young_modulus/pyc/bison_2021_pyc_young_modulus.hpp"

namespace trimuse
{

Bison2021PycYoungModulus::Bison2021PycYoungModulus(const ModelParameterMap&)
{
}

// Jiang et al., Journal of Nuclear Materials 548 (2021) 152795，式（24–25）
VectorValue Bison2021PycYoungModulus::evaluate(const PropertiesStatePoint& state_point) const
{
    const double temperature_celsius = state_point.scalar("temperature") - 273.15;
    const double density = state_point.scalar("material.general.mass_density");
    const double initial_baf = state_point.scalar("material.general.initial_bacon_anisotropy_factor");
    const double crystallite_diameter = state_point.scalar("material.general.crystallite_diameter") * 1e10; // m -> Å
    const double fluence = state_point.scalar("derived.fast_neutron_fluence") / 1e25;
    const double modulus = 25.5e9 * (0.384 + 0.324e-3 * density) * (2.985 - 0.0662 * crystallite_diameter) * (1.0 + 0.23 * fluence) * (1.0 + 1.5e-4 * (temperature_celsius - 20.0));
    const double er = modulus * (1.463 - 0.463 * initial_baf);
    const double et = modulus * (0.481 + 0.519 * initial_baf);
    return {er, et};
}

}
