#include "properties/assembly/cumulative_fission_gas_release/from_homogenized_booth_fission_gas_release.hpp"
#include "properties/assembly/homogenized_booth_fission_gas_release_state/booth.hpp"

namespace trimuse
{

FromHomogenizedBoothFissionGasRelease::FromHomogenizedBoothFissionGasRelease(const ModelParameterMap&)
{
}

double FromHomogenizedBoothFissionGasRelease::evaluate(const PropertiesStatePoint& point) const
{
    return point.vector("assembly.homogenized_booth_fission_gas_release_state")[HomogenizedBoothFissionGasReleaseState::released_density] * point.scalar("derived.region_volume"); // mol，区域等效释放量密度乘区域体积
}

}
