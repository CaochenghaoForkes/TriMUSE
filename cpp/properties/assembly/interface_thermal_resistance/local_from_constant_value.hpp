#pragma once

#include "common/angular_range.hpp"

#include "properties/assembly/interface_thermal_resistance/interface_thermal_resistance_model_manager.hpp"

namespace trimuse
{

class LocalInterfaceThermalResistance final : public InterfaceThermalResistance
{
public:
    explicit LocalInterfaceThermalResistance(const ModelParameterMap& parameters);

    std::optional<double> evaluate(const PropertiesStatePoint& point) const override;

private:
    AngularRanges angles_; // 存放角域的 std::vector 直接通过含约定好的角域信息的 parametersmap 构造
    std::vector<double> resistances_;
};

}
