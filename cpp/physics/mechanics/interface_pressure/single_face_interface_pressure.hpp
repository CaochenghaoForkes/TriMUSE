#pragma once

#include "physics/mechanics/interface_pressure/mechanics_interface_pressure_model.hpp"

namespace trimuse
{

class SingleFaceInterfacePressure final : public MechanicsInterfacePressureModel
{
public:
    explicit SingleFaceInterfacePressure(const ModelParameterMap& parameters);
    std::vector<MechanicsInterfacePressureValue> evaluate(const PropertiesFieldData& properties, const MeshData::Interface& interface) const override;

private:
    int region_attribute_; // Python 将属性卡中的受压侧区域名称转换为编号
};

}
