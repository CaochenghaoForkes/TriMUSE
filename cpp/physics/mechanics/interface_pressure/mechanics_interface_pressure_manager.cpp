#include "physics/mechanics/interface_pressure/mechanics_interface_pressure_manager.hpp"
#include "physics/mechanics/interface_pressure/single_face_interface_pressure.hpp"
#include "physics/mechanics/interface_pressure/double_face_interface_pressure.hpp"
#include "physics/mechanics/interface_pressure/mechanics_interface_pressure_assembly.hpp"

namespace trimuse
{

MechanicsInterfacePressureManager::MechanicsInterfacePressureManager(const FemSystem& fem, const std::vector<MechanicsInterfaceConfig>& configs)
{
    registry_.register_model<SingleFaceInterfacePressure>("single_face", {"assembly.interface_pressure"}); // 注册
    registry_.register_model<DoubleFaceInterfacePressure>("double_face", {"assembly.interface_pressure"});
    for (const auto& config : configs) // 安排并初始化各界面区域对应的 model required faces(vector{面片几何索引,压力})
    {
        const auto& entry = registry_.registration(config.model_name);
        interface_pressure_assignments_.emplace(config.boundary_attribute, Assignment{entry.factory(config.parameters), entry.required_states, {}});
    }
    for (const auto& interface : fem.mesh_data().interfaces()) // 遍历界面面片
    {
        const auto assignment = interface_pressure_assignments_.find(interface.boundary_attribute);
        if (assignment != interface_pressure_assignments_.end()) // 如果当前面片安排了模型
        {
            assignment->second.faces.push_back({&interface}); // 只登记配置了压力模型的界面面片
        }
    }
}

void MechanicsInterfacePressureManager::update(const PropertiesFieldData& properties)
{
    for (auto& [attribute, assignment] : interface_pressure_assignments_) // 遍历有安排界面压力模型的界面区域
    {
        for (auto& face : assignment.faces) // 遍历该区域中的面片
        {
            const auto values = assignment.model->evaluate(properties, *face.interface); // 取得当前面片各侧的压力贡献
            assignment.dirty = assignment.dirty || values != face.values; // 更新 dirty
            face.values = values; // 更新值
        }
    }
}

void MechanicsInterfacePressureManager::mark_coupled(PropertiesSystem& properties, const std::string& field) const
{
    for (const auto& [attribute, assignment] : interface_pressure_assignments_)
    {
        properties.mark_coupled(field, assignment.required);
    }
}

bool MechanicsInterfacePressureManager::coupled(const PropertiesSystem& properties, const std::string& field) const
{
    for (const auto& [attribute, assignment] : interface_pressure_assignments_)
    {
        for (const auto& dependency : assignment.required)
        {
            const std::string& name = dependency.name;
            if (properties.coupled_with_field(name, attribute, field))
            {
                return true;
            }
        }
    }
    return false;
}

std::vector<int> MechanicsInterfacePressureManager::interface_pressure_attributes() const
{
    std::vector<int> attributes;
    for (const auto& [attribute, assignment] : interface_pressure_assignments_)
    {
        attributes.push_back(attribute);
    }
    return attributes;
}

bool MechanicsInterfacePressureManager::required_no_change(int attribute) const
{
    return !interface_pressure_assignments_.at(attribute).dirty;
}

void MechanicsInterfacePressureManager::assembled(int attribute)
{
    interface_pressure_assignments_.at(attribute).dirty = false;
}

const std::vector<MechanicsInterfacePressureFace>& MechanicsInterfacePressureManager::values(int attribute) const
{
    return interface_pressure_assignments_.at(attribute).faces;
}

std::unique_ptr<mfem::Vector> MechanicsInterfacePressureManager::assemble_load(int attribute, FemSystem& fem, const TrialField& trial_field, const PropertiesFieldData::FaceIntegrationPointValues& weights) const // 组装指定界面 attribute 的完整压力载荷向量
{
    const auto& assignment = interface_pressure_assignments_.at(attribute);
    MechanicsInterfacePressureAssembly assembly(fem, trial_field); // 创建积分组装器，将完整载荷向量清零
    for (const auto& face : assignment.faces) // 遍历各面片
    {
        const InterfaceContext context(*face.interface, fem);
        for (const auto& value : face.values) // 再遍历该面片的单侧或双侧压力贡献
        {
            assembly.add_pressure(context, value.side, value.pressure, weights); // 将各贡献积分并累加
        }
    }
    return assembly.finalize(); // 返回汇总后的载荷向量
}

}
