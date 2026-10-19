#include "properties/derived/volumetric_power/uniform.hpp"

namespace trimuse
{

UniformVolumetricPower::UniformVolumetricPower(const ModelParameterMap&)
{
}

StateValue UniformVolumetricPower::evaluate(const PropertiesStatePoint& point) const
{
    const double power = point.scalar("power"); // 区域总功率, W
    const double region_volume = point.scalar("region_volume"); // 上层提供的当前区域物理体积, m³
    return power / region_volume; // 区域内均匀功率密度, W/m³
}

}
