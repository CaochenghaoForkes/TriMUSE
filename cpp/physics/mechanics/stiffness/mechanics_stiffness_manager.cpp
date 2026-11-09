#include "physics/mechanics/stiffness/mechanics_stiffness_manager.hpp"
#include "physics/mechanics/stiffness/linear_elastic_stiffness.hpp"

namespace trimuse
{

MechanicsStiffnessManager::MechanicsStiffnessManager(const FemSystem& fem, const std::vector<MechanicsDomainConfig>& configs) // 注册并按排弹性张量模型
    : fem_(fem), values_(fem.domain_quadrature_point_count())
{
    registry_.register_model<LinearElasticStiffness>("linear_elastic", {"material.mechanics.elasticity_tensor"});
    auto assignments = configs;
    if (assignments.empty())
    {
        for (int region : fem_.mesh_data().region_attributes()) 
        { 
            assignments.push_back({region, "linear_elastic", {}});  // 默认为线弹性本构
        }
    }
    for (const auto& config : assignments)
    {
        const auto& entry = registry_.registration(config.model_name);
        models_.emplace(config.region_attribute, Assignment{entry.factory(config.parameters), entry.required_states});
    }
}

void MechanicsStiffnessManager::update(const PropertiesFieldData& properties) // 更新弹性张量全局积分点向量以及对应的 dirty
{
    for (auto& [region, assignment] : models_)
    for (int element : fem_.mesh_data().region_elements(region))
    for (std::size_t point = fem_.domain_quadrature_offset(element); point < fem_.domain_quadrature_offset(element + 1); ++point)
    {
        const auto value = assignment.model->evaluate(properties, point);
        assignment.dirty = assignment.dirty || value != values_[point];
        values_[point] = value;
    }
}

bool MechanicsStiffnessManager::coupled(const PropertiesSystem& properties, const std::string& field) const // 判断: 当前管理器负责的模型中，是否有任何一个模型的依赖属性与指定求解场耦合
{
    for (const auto& [region, assignment] : models_)
    for (const auto& name : assignment.required)
    {
        if (properties.coupled_with_field(name, region, field)) 
        { 
            return true; 
        }
    }
    return false;
}

void MechanicsStiffnessManager::mark_coupled(PropertiesSystem& properties, const std::string& field) const // 把当前管理器所依赖的属性，交给属性系统登记与指定场的耦合关系
{
    for (const auto& [region, assignment] : models_)
    {
        properties.mark_coupled(field, assignment.required);
    }
}

bool MechanicsStiffnessManager::required_no_change(int region) const // 根据 dirty 判断指定区域的系数是否与最近一次组装时一致(dirty是否为False)，因而可以复用已有矩阵或向量
{
    return !models_.at(region).dirty;
}

void MechanicsStiffnessManager::assembled(int region) // 将 region 处的 dirty 记为 false
{
    models_.at(region).dirty = false;
}

std::vector<int> MechanicsStiffnessManager::regions() const // 返回当前管理器中已经配置模型的所有区域编号
{
    std::vector<int> result;
    for (const auto& [region, assignment] : models_) 
    { 
        result.push_back(region); 
    }
    return result;
}

}
