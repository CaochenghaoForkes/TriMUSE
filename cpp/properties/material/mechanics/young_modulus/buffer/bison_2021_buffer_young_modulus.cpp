#include "properties/material/mechanics/young_modulus/buffer/bison_2021_buffer_young_modulus.hpp"

namespace trimuse
{

Bison2021BufferYoungModulus::Bison2021BufferYoungModulus(const ModelParameterMap&)
{
}

// Jiang et al., Journal of Nuclear Materials 548 (2021) 152795，式（14）
VectorValue Bison2021BufferYoungModulus::evaluate(const PropertiesStatePoint& state_point) const
{
    const double temperature_celsius = state_point.scalar("temperature") - 273.15;
    const double density = state_point.scalar("material.general.mass_density");
    const double fluence = state_point.scalar("derived.fast_neutron_fluence") / 1e25;
    const double modulus = 25.5e9 * (0.384 + 0.324e-3 * density) * (1.0 + 0.23 * fluence) * (1.0 + 1.5e-4 * (temperature_celsius - 20.0));
    return {modulus, modulus};
}

}
