#include "properties/assembly/interface_thermal_resistance/local_from_constant_value.hpp"

namespace trimuse
{

LocalInterfaceThermalResistance::LocalInterfaceThermalResistance(const ModelParameterMap& parameters)
    :
    angles_(parameters),
    resistances_(AngularRanges::number_array(parameters, "constant_value"))
{
}

std::optional<double> LocalInterfaceThermalResistance::evaluate(const PropertiesStatePoint& point) const
{
    const auto& center = point.vector("precomputed.geometry_center");
    const auto range = angles_.match({point.x - center[0], point.y - center[1], point.z - center[2]});
    if (range)
    {
        return resistances_[*range];
    }
    return std::nullopt;
}

}
