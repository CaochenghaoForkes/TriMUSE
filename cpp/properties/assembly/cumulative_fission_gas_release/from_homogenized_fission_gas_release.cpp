#include "properties/assembly/cumulative_fission_gas_release/from_homogenized_fission_gas_release.hpp"
#include "properties/assembly/homogenized_fission_gas_release_state/booth.hpp"

namespace trimuse
{

FromHomogenizedFissionGasRelease::FromHomogenizedFissionGasRelease(const ModelParameterMap&)
{
}

double FromHomogenizedFissionGasRelease::evaluate(const PropertiesStatePoint& point) const
{
    return point.vector("assembly.homogenized_fission_gas_release_state")[HomogenizedFissionGasReleaseState::released_density] * point.scalar("derived.region_volume"); // mol，区域等效释放量密度乘区域体积
}

}
