#include "physics/thermal/interface_heat_transfer/thermal_interface_heat_transfer_manager.hpp"
#include "physics/thermal/interface_heat_transfer/interface_thermal_resistance.hpp"
#include "physics/thermal/interface_heat_transfer/perfect_contact.hpp"
#include "physics/thermal/interface_heat_transfer/thermal_interface_heat_transfer_assembly.hpp"
#include "properties/properties_field_data.hpp"

#include <map>
#include <stdexcept>

#include "common/model_registry.hpp"

namespace trimuse
{

ThermalInterfaceManager::ThermalInterfaceManager(FemSystem& fem, TrialField& trial_field, DofConstraints& constraints, const std::vector<ThermalInterfaceConfig>& interface_heat_transfer_configs)
    :
    fem_(fem),
    values_(fem.face_quadrature_point_count(), 0.0)
{
    ModelRegistry<ThermalInterfaceModel> registry;
    registry.register_model<ThermalPerfectContact>("perfect_contact");
    registry.register_model<ThermalResistanceFromProperty>("interface_thermal_resistance");

    // 给每个材料界面安排热界面模型
    std::map<int, std::unique_ptr<ThermalInterfaceModel>> models;
    for (const auto& config : interface_heat_transfer_configs)
    {
        auto model = registry.registration(config.model_name).factory(config.parameters);
        if (!trial_field.regional() && model->requires_region_field())
        {
            throw std::invalid_argument("所选热界面模型需要分域温度场: " + config.model_name);
        }
        models.emplace(config.boundary_attribute, std::move(model));
    }

    // 完美接触的面片调用 assembly.connect(context)，登记两侧对应温度自由度的相等关系
    // 按材料界面编号登记热阻面片，两侧温度自由度保持独立
    ThermalInterfaceAssembly assembly(trial_field, constraints);
    for (const auto& interface : fem.mesh_data().interfaces())
    {
        const auto model = models.find(interface.boundary_attribute);
        const InterfaceContext context(interface, fem);
        if (model == models.end() || model->second->is_perfect_contact(context))
        {
            assembly.connect(context);
        }
        else
        {
            interface_heat_transfer_assignments_[interface.boundary_attribute].faces.push_back(&interface); // 有热阻的面片才需要被记录进 interface_heat_transfer_assignments_[interface.boundary_attribute]
        }
    }
}

void ThermalInterfaceManager::update(const PropertiesFieldData& properties) // 逐面积分点读取最新热阻，并按材料界面标记 dirty
{
    if (interface_heat_transfer_assignments_.empty()) // 如果没有登记任何热阻面片，就直接返回
    {
        return;
    }
    const auto& values = properties.face_integration_point_values.at("assembly.interface_thermal_resistance"); // 取得热阻属性表的只读引用
    for (auto& [attribute, assignment] : interface_heat_transfer_assignments_)
        for (const auto* face : assignment.faces) // 遍历当前材料界面中需要热阻组装的面片
            for (std::size_t point = fem_.face_quadrature_offset(face->face); point < fem_.face_quadrature_offset(face->face + 1); ++point)
            {
                const double resistance = std::get<double>(values[point].value());
                assignment.dirty = assignment.dirty || values_[point] != resistance;
                values_[point] = resistance;
            }
}

std::vector<int> ThermalInterfaceManager::interface_heat_transfer_attributes() const // 返回所有有热阻面片的界面编号列表
{
    std::vector<int> attributes;
    for (const auto& [attribute, assignment] : interface_heat_transfer_assignments_)
    {
        attributes.push_back(attribute);
    }
    return attributes;
}

bool ThermalInterfaceManager::required_no_change(int attribute) const // 查询一个界面区域是不是 dirty
{
    return !interface_heat_transfer_assignments_.at(attribute).dirty;
}

void ThermalInterfaceManager::assembled(int attribute) // 清理一个界面区域的 dirty 为 false
{
    interface_heat_transfer_assignments_.at(attribute).dirty = false;
}

std::unique_ptr<mfem::SparseMatrix> ThermalInterfaceManager::assemble_matrix(int attribute, FemSystem& fem, const TrialField& trial_field, DofConstraints& constraints, const PropertiesFieldData::FaceIntegrationPointValues& weights) const // 组装编号为 attribute 的界面传热矩阵
{
    ThermalInterfaceAssembly assembly(trial_field, constraints);
    for (const auto* face : interface_heat_transfer_assignments_.at(attribute).faces)
    {
        const InterfaceContext context(*face, fem);
        assembly.add_transfer(context, values_, weights);
    }
    return assembly.finalize();
}

}
