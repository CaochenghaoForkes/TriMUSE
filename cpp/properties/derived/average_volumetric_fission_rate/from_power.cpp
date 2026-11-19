#include "properties/derived/average_volumetric_fission_rate/from_power.hpp"

namespace trimuse
{

FromPower::FromPower(const ModelParameterMap&)
{
}

StateValue FromPower::evaluate(const PropertiesStatePoint& point) const
{
    const double power = point.scalar("external.power"); // 区域总功率, W
    const double region_volume = point.scalar("derived.region_volume"); // 上层提供的当前区域物理体积, m³
    const double energy_per_fission = point.scalar("external.energy_per_fission"); // 单次裂变释热能量, J
    return power / (region_volume * energy_per_fission); // 区域平均体积裂变率, 1/(m³·s)
}

}
