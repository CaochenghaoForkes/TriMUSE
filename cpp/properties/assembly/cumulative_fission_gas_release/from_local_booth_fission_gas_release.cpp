#include "properties/assembly/cumulative_fission_gas_release/from_local_booth_fission_gas_release.hpp"
#include "properties/assembly/local_booth_fission_gas_release_state/booth.hpp"

namespace trimuse
{

FromLocalBoothFissionGasRelease::FromLocalBoothFissionGasRelease(const ModelParameterMap&)
{
}

double FromLocalBoothFissionGasRelease::evaluate(const PropertiesStatePoint& point) const
{
    const auto& states = point.domain_values("assembly.local_booth_fission_gas_release_state");
    const auto& weights = point.domain_values("precomputed.initial_domain_quadrature_weight");
    double released = 0.0;
    for (std::size_t index : point.region_point_indices())
    {
        released += std::get<double>(weights[index].value()) * std::get<VectorValue>(states[index].value())[LocalBoothFissionGasReleaseState::released_density]; // Nrel = Σ wq nrel,q，单位 mol
    }
    return released;
}

}
