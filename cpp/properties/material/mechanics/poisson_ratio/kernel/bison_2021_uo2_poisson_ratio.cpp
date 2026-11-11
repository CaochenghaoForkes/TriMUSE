#include "properties/material/mechanics/poisson_ratio/kernel/bison_2021_uo2_poisson_ratio.hpp"

namespace trimuse
{

Bison2021Uo2PoissonRatio::Bison2021Uo2PoissonRatio(const ModelParameterMap&)
{
}

// Jiang et al., Journal of Nuclear Materials 548 (2021) 152795，式（9）
VectorValue Bison2021Uo2PoissonRatio::evaluate(const PropertiesStatePoint& state_point) const
{
    constexpr double theoretical_mass_density = 10960.0; // kg/m³
    const double density = state_point.scalar("material.general.mass_density");
    const double ratio = 1.35 * (1.92 * density - 0.92 * theoretical_mass_density) / (1.66 * density - 0.66 * theoretical_mass_density) - 1.0;
    return {ratio, ratio};
}

}
