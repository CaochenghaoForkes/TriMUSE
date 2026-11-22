#pragma once

#include <map>

#include "common/model_registry.hpp"
#include "fem/fem_system.hpp"
#include "physics/mechanics/interface_pressure/mechanics_interface_pressure_model.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

struct MechanicsInterfacePressureFace // 一个面片的几何索引与当前压力
{
    const MeshData::Interface* interface;
    std::vector<MechanicsInterfacePressureValue> values; // 可为空、单侧或双侧压力列表
};

class TrialField;

class MechanicsInterfacePressureManager
{
public:
    MechanicsInterfacePressureManager(const FemSystem& fem, const std::vector<MechanicsInterfaceConfig>& configs);
    void update(const PropertiesFieldData& properties);
    void mark_coupled(PropertiesSystem& properties, const std::string& field) const;
    bool coupled(const PropertiesSystem& properties, const std::string& field) const;
    std::vector<int> interface_pressure_attributes() const;
    bool required_no_change(int attribute) const;
    void assembled(int attribute);
    std::unique_ptr<mfem::Vector> assemble_load(int attribute, FemSystem& fem, const TrialField& trial_field, const PropertiesFieldData::FaceIntegrationPointValues& weights) const;
    const std::vector<MechanicsInterfacePressureFace>& values(int attribute) const;

private:
    struct Assignment // model required faces(vector{面片几何索引,压力})
    {
        std::unique_ptr<MechanicsInterfacePressureModel> model;
        std::vector<RequiredState> required;
        std::vector<MechanicsInterfacePressureFace> faces;
        bool dirty = true;
    };

    ModelRegistry<MechanicsInterfacePressureModel> registry_;
    std::map<int, Assignment> interface_pressure_assignments_; // 界面物理编号 -> 模型、面片压力和组装标记
};

}
