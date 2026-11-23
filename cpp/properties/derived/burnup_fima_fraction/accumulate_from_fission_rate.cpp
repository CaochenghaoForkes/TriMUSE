#include "properties/derived/burnup_fima_fraction/accumulate_from_fission_rate.hpp"

namespace trimuse
{

AccumulateFromFissionRate::AccumulateFromFissionRate(const ModelParameterMap& parameters)
    :
    initial_value_(std::get<double>(parameters.at("initial_value")))
{
}

StateValue AccumulateFromFissionRate::evaluate(const PropertiesStatePoint& point) const
{
    const double previous = point.previous_value ? std::get<double>(*point.previous_value) : initial_value_;
    const double fission_rate = point.scalar("derived.average_volumetric_fission_rate");
    const double volume = point.scalar("derived.region_volume");
    const double inventory = point.scalar("external.initial_heavy_metal_inventory"); // mol，初始重金属物质的量
    constexpr double avogadro_constant = 6.02214076e23; // mol⁻¹
    return previous + point.scalar("external.time_step") * fission_rate * volume / (avogadro_constant * inventory); // Bⁿ⁺¹ = Bⁿ + Δt f‴ V / (N_A n_HM,0)
}

}
