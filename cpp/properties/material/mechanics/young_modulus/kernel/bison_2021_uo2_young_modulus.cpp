#include "properties/material/mechanics/young_modulus/kernel/bison_2021_uo2_young_modulus.hpp"

namespace trimuse
{

Bison2021Uo2YoungModulus::Bison2021Uo2YoungModulus(const ModelParameterMap&)
{
}

// Jiang et al., Journal of Nuclear Materials 548 (2021) 152795，式（6）
VectorValue Bison2021Uo2YoungModulus::evaluate(const PropertiesStatePoint& state_point) const
{
    constexpr double theoretical_mass_density = 10960.0; // kg/m³
    const double temperature_celsius = state_point.scalar("temperature") - 273.15;
    const double density = state_point.scalar("material.general.mass_density");
    const double t2 = temperature_celsius * temperature_celsius;
    const double temperature_factor = 1.0 - 1.1e-4 * temperature_celsius - 2.1e-7 * t2 + 3.1e-10 * t2 * temperature_celsius - 1.6e-13 * t2 * t2;
    // 密度比中统一使用 kg/m³，公式输出由 GPa 换算为 Pa
    const double modulus = 219e9 * temperature_factor * (1.92 * density - 0.92 * theoretical_mass_density) / theoretical_mass_density;
    return {modulus, modulus};
}

}
