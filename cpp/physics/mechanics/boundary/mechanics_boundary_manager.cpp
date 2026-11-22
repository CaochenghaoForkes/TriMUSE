#include "physics/mechanics/boundary/mechanics_boundary_manager.hpp"
#include "physics/mechanics/boundary/from_surface_traction.hpp"
#include "physics/mechanics/boundary/from_surface_displacement.hpp"
#include "physics/mechanics/boundary/from_surface_pressure.hpp"

namespace trimuse
{

MechanicsBoundaryManager::MechanicsBoundaryManager(const std::vector<MechanicsBoundaryConfig>& configs) // 注册与安排各 Zone 边界模型
{
    registry_.register_model<MechanicsFromSurfaceTraction>("from_surface_traction", {"external.surface_traction"});
    registry_.register_model<MechanicsFromSurfaceDisplacement>("from_surface_displacement", {"external.surface_displacement"});
    registry_.register_model<MechanicsFromSurfacePressure>("from_surface_pressure", {"external.surface_pressure"});
    for (const auto& config : configs)
    {
        const auto& entry = registry_.registration(config.model_name);
        models_.emplace(config.boundary_attribute, Assignment{entry.factory(config.parameters), entry.required_states});
    }
}

void MechanicsBoundaryManager::update(const PropertiesFieldData& properties) // 更新各 Zone 边界系数与 dirty
{
    for (auto& [zone, assignment] : models_)
    {
        auto value = assignment.model->evaluate(properties, zone);
        assignment.dirty = assignment.dirty || value != assignment.value;
        assignment.value = std::move(value);
    }
}

void MechanicsBoundaryManager::mark_coupled(PropertiesSystem& properties, const std::string& field) const // 标记由于边界而与位移场耦合的属性
{
    for (const auto& [zone, assignment] : models_)
    {
        properties.mark_coupled(field, assignment.required);
    }
}

bool MechanicsBoundaryManager::coupled(const PropertiesSystem& properties, const std::string& field) const // 判断属性是否因边界值而与位移场耦合
{
    for (const auto& [zone, assignment] : models_)
        for (const auto& dependency : assignment.required)
        {
            const std::string& name = dependency.name;
            if (properties.coupled_with_field(name, zone, field))
            {
                return true;
            }
        }
    return false;
}

bool MechanicsBoundaryManager::required_no_change(int zone) const // 根据 dirty 判断指定区域的系数是否与最近一次组装时一致(dirty是否为False)，因而可以复用已有矩阵或向量
{
    return !models_.at(zone).dirty;
}

void MechanicsBoundaryManager::assembled(int zone) // 将 region 处的 dirty 记为 false
{
    models_.at(zone).dirty = false;
}

std::vector<int> MechanicsBoundaryManager::zones() const // 返回当前管理器中已经配置模型的所有边界区域编号
{
    std::vector<int> result;
    for (const auto& [zone, assignment] : models_)
    {
        result.push_back(zone);
    }
    return result;
}

const MechanicsBoundaryValue& MechanicsBoundaryManager::values(int zone) const // 指定边界区域 返回不同模型的计算结果 面力、位移保存求解坐标系中的向量，压力保存标量；每个分区只有一种边界条件
{
    return models_.at(zone).value;
}

MechanicsBoundaryType MechanicsBoundaryManager::type(int zone) const // 指定边界区域的模型的返回类型 三类力学边界 0-Traction 面力向量 1-Displacement 各向位移 2-Pressure 法向压力
{
    return models_.at(zone).model->type();
}

bool MechanicsBoundaryManager::essential(int zone) const // 判断指定边界区域是否为给定位移的本质边界
{
    return type(zone) == MechanicsBoundaryType::Displacement;
}

}
