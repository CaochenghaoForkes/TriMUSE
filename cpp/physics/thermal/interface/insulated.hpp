#pragma once

#include "physics/thermal/interface/thermal_interface_model.hpp"

namespace trimuse
{

class ThermalInsulatedInterface final : public ThermalInterfaceModel
{
public:
    explicit ThermalInsulatedInterface(const ModelParameterMap&) {}
    
    void assemble(const ThermalInterfaceContext& context, ThermalInterfaceAssembly& assembly) const override;
};

}
