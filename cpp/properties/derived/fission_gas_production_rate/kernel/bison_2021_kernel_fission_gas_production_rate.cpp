#include "properties/derived/fission_gas_production_rate/kernel/bison_2021_kernel_fission_gas_production_rate.hpp"

namespace trimuse
{

Bison2021KernelFissionGasProductionRate::Bison2021KernelFissionGasProductionRate(const ModelParameterMap&)
{
}

StateValue Bison2021KernelFissionGasProductionRate::evaluate(const PropertiesStatePoint& point) const
{
    constexpr double gas_yield = 0.297; // 每次裂变产生的气体原子数
    constexpr double avogadro_constant = 6.02214076e23; // mol⁻¹
    const double fission_rate = point.scalar("derived.average_volumetric_fission_rate");
    const double volume = point.scalar("derived.region_volume");
    return gas_yield * fission_rate * volume / avogadro_constant; // 区域裂变气体总产生率, mol/s
}

}
