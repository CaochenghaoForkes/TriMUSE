#pragma once

#include "physics/thermal/interface/thermal_interface_model.hpp"

namespace trimuse
{

class ThermalUniformFromConstantResistance final : public ThermalInterfaceModel
{
public:
    explicit ThermalUniformFromConstantResistance(const ModelParameterMap& parameters);
    void assemble(const InterfaceContext& context, ThermalInterfaceAssembly& assembly) const override;

private:
    double resistance_; // 单位面积热阻, m²·K/W
};

}
