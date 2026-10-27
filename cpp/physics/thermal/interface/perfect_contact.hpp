#pragma once

#include "physics/thermal/interface/thermal_interface_model.hpp"

namespace trimuse
{

class ThermalPerfectContact final : public ThermalInterfaceModel
{
public:
    explicit ThermalPerfectContact(const ModelParameterMap&) {}

    bool requires_region_field() const override { return false; }
    
    void assemble(const ThermalInterfaceContext& context, ThermalInterfaceAssembly& assembly) const override;
};

}
