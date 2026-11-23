#pragma once

#include "physics/mechanics/interface_pressure/mechanics_interface_pressure_model.hpp"

namespace trimuse
{

class MechanicsFromInternalGasPressure final : public MechanicsInterfacePressureModel
{
public:
    explicit MechanicsFromInternalGasPressure(const ModelParameterMap& parameters);
    MechanicsInterfacePressureValue evaluate(const PropertiesFieldData& properties, const MeshData::Interface& interface) const override;

private:
    int gas_region_;
};

}
