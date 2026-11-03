#include "physics/radioactivity/boundary/radioactivity_boundary_manager.hpp"
#include "physics/radioactivity/boundary/radioactivity_dirichlet_boundary.hpp"
#include "physics/radioactivity/boundary/radioactivity_neumann_boundary.hpp"
#include "physics/radioactivity/boundary/radioactivity_robin_boundary.hpp"

namespace trimuse
{

RadioactivityBoundaryManager::RadioactivityBoundaryManager(const std::vector<RadioactivityBoundaryConfig>& configs) // 注册三类边界模型并按分区配置
{
    registry_.register_model<RadioactivityDirichletBoundary>("dirichlet", {"external.surface_nuclide_concentration"});
    registry_.register_model<RadioactivityNeumannBoundary>("neumann", {"external.nuclide_flux"});
    registry_.register_model<RadioactivityRobinBoundary>("robin", {"derived.mass_transfer_coef", "external.environment_nuclide_concentration"});
    for (const auto& config : configs)
    {
        const auto& entry = registry_.registration(config.model_name);
        models_.emplace(config.boundary_attribute, Assignment{entry.factory(config.parameters), entry.required_states});
    }
}

void RadioactivityBoundaryManager::update(const PropertiesFieldData& properties) // 分别比较矩阵、右端和指定浓度，保留尚未处理的 dirty 标记
{
    for (auto& [zone, assignment] : models_)
    {
        const auto value = assignment.model->evaluate(properties, zone);
        assignment.dirty[0] = assignment.dirty[0] || value.matrix != assignment.values.matrix;
        assignment.dirty[1] = assignment.dirty[1] || value.load != assignment.values.load;
        assignment.dirty[2] = assignment.dirty[2] || value.concentration != assignment.values.concentration;
        assignment.values = value;
    }
}

void RadioactivityBoundaryManager::mark_coupled(PropertiesSystem& properties, const std::string& field) const // 交由属性系统识别实际耦合区域
{
    for (const auto& [zone, assignment] : models_)
    {
        properties.mark_coupled(field, assignment.required);
    }
}

bool RadioactivityBoundaryManager::coupled(const PropertiesSystem& properties, const std::string& field) const // 任一边界所需属性参与场耦合即返回 true
{
    for (const auto& [zone, assignment] : models_)
    for (const auto& name : assignment.required)
    {
        if (properties.coupled_with_field(name, zone, field))
        {
            return true;
        }
    }
    return false;
}

bool RadioactivityBoundaryManager::required_no_change(int zone, RadioactivityBoundaryPart part) const // dirty 为 false 时可复用该部分
{
    return !models_.at(zone).dirty[static_cast<int>(part)];
}

void RadioactivityBoundaryManager::assembled(int zone, RadioactivityBoundaryPart part) // 某一部分组装或施加完后单独清除标记
{
    models_.at(zone).dirty[static_cast<int>(part)] = false;
}

std::vector<int> RadioactivityBoundaryManager::zones() const // 返回模型映射表中的边界分区编号
{
    std::vector<int> result;
    for (const auto& [zone, assignment] : models_)
    {
        result.push_back(zone);
    }
    return result;
}

const RadioactivityBoundaryValues& RadioactivityBoundaryManager::values(int zone) const
{
    return models_.at(zone).values;
}

bool RadioactivityBoundaryManager::essential(int zone) const
{
    return models_.at(zone).model->essential();
}

}
