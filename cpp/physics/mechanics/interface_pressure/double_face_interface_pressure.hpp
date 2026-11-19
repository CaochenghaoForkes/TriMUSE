#pragma once

#include "physics/mechanics/interface_pressure/mechanics_interface_pressure_model.hpp"

namespace trimuse
{

class DoubleFaceInterfacePressure final : public MechanicsInterfacePressureModel
{
public:
    explicit DoubleFaceInterfacePressure(const ModelParameterMap& parameters);
    std::vector<MechanicsInterfacePressureValue> evaluate(const PropertiesFieldData& properties, const MeshData::Interface& interface) const override;
};

}
