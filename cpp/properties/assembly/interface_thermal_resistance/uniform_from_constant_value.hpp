#pragma once

#include "properties/assembly/interface_thermal_resistance/interface_thermal_resistance_model_manager.hpp"

namespace trimuse
{

class UniformInterfaceThermalResistance final : public InterfaceThermalResistance
{
public:
    explicit UniformInterfaceThermalResistance(const ModelParameterMap& parameters);

    std::optional<double> evaluate(const PropertiesStatePoint& point) const override;

private:
    double resistance_;
};

}
