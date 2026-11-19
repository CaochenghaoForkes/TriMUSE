#include "properties/assembly/interface_thermal_resistance/uniform_from_constant_value.hpp"

namespace trimuse
{

UniformInterfaceThermalResistance::UniformInterfaceThermalResistance(const ModelParameterMap& parameters)
    :
    resistance_(std::get<double>(parameters.at("constant_value")))
{
}

std::optional<double> UniformInterfaceThermalResistance::evaluate(const PropertiesStatePoint&) const
{
    return resistance_;
}

}
