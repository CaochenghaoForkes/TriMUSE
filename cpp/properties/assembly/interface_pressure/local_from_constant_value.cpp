#include "properties/assembly/interface_pressure/local_from_constant_value.hpp"

namespace trimuse
{

LocalInterfacePressure::LocalInterfacePressure(const ModelParameterMap& parameters)
    :
    angles_(parameters),
    pressures_(AngularRanges::number_array(parameters, "constant_value"))
{
}

double LocalInterfacePressure::evaluate(const PropertiesStatePoint& point) const
{
    const auto& center = point.vector("precomputed.geometry_center");
    const auto range = angles_.match({point.x - center[0], point.y - center[1], point.z - center[2]});
    if (range)
    {
        return pressures_[*range]; // 多个角域重叠时，采用输入顺序中首先命中的压力
    }
    return 0.0; // 角域外不施加压力
}

}
