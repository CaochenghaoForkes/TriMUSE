#include "properties/derived/volumetric_nuclide_birth_rate/uniform_from_birth_rate.hpp"

namespace trimuse
{

UniformFromBirthRate::UniformFromBirthRate(const ModelParameterMap&)
{
}

StateValue UniformFromBirthRate::evaluate(const PropertiesStatePoint& point) const
{
    const double birth_rate = point.scalar("external.nuclide_birth_rate"); // 全局产生率, atom/s
    const double fraction = point.scalar("external.uranium_fraction"); // 当前区域占全局产生率的份额
    const double volume = point.scalar("precomputed.region_volume"); // 当前区域物理体积, m³
    return birth_rate * fraction / volume; // 区域内均匀体积产生率, atom/(m³·s)
}

}
