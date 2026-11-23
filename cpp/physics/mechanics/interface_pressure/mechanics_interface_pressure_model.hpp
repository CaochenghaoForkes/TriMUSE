#pragma once

#include "physics/mechanics/common/mechanics_interface_config.hpp"
#include "fem/mesh_data.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

struct MechanicsInterfacePressureValue // 当前面片的受压侧及压力
{
    int side = 0; // 对应 Interface::sides 的 0 或 1
    double pressure = 0.0;

    bool operator==(const MechanicsInterfacePressureValue& other) const { return side == other.side && pressure == other.pressure; } // 比较受压侧及压力，更新 dirty
};

class MechanicsInterfacePressureModel // 读取区域气压并提供受压侧，不改变界面连接关系
{
public:
    virtual ~MechanicsInterfacePressureModel() = default;
    virtual MechanicsInterfacePressureValue evaluate(const PropertiesFieldData& properties, const MeshData::Interface& interface) const = 0;
};

}
