#include "physics/mechanics/source/mechanics_source_manager.hpp"
#include "physics/mechanics/source/gravity.hpp"

namespace trimuse
{

MechanicsSourceManager::MechanicsSourceManager(const FemSystem& fem, const std::vector<MechanicsDomainConfig>& configs)
    : fem_(fem), values_(fem.domain_quadrature_point_count(), VectorValue(fem.mesh_data().dimension(), 0.0))
{
    registry_.register_model<GravitySource>("gravity", {"material.general.mass_density", "precomputed.gravity_acceleration"}); // 注册
    for (const auto& config : configs)
    {
        const auto& entry = registry_.registration(config.model_name);
        models_.emplace(config.region_attribute, Assignment{entry.factory(config.parameters), entry.required_states}); // 安排
    }
}

void MechanicsSourceManager::update(const PropertiesFieldData& properties) // 更新各个求积点上的体积力与 dirty 标记
{
    for (auto& [region, assignment] : models_)
    for (int element : fem_.mesh_data().region_elements(region))
    for (std::size_t point = fem_.domain_quadrature_offset(element); point < fem_.domain_quadrature_offset(element + 1); ++point)
    {
        auto force = assignment.model->evaluate(properties, point);
        assignment.dirty = assignment.dirty || force != values_[point];
        values_[point] = std::move(force);
    }
}

bool MechanicsSourceManager::coupled(const PropertiesSystem& properties, const std::string& field) const // 判断属性是否因体积力而与位移场耦合
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

void MechanicsSourceManager::mark_coupled(PropertiesSystem& properties, const std::string& field) const // 标记由于体积力而与位移场耦合的属性
{
    for (const auto& [region, assignment] : models_)
    {
        properties.mark_coupled(field, assignment.required);
    }
}

bool MechanicsSourceManager::required_no_change(int region) const // 根据 dirty 判断指定区域的系数是否与最近一次组装时一致(dirty是否为False)，因而可以复用已有矩阵或向量
{
    return !models_.at(region).dirty;
}

void MechanicsSourceManager::assembled(int region) // 将 region 处的 dirty 记为 false
{
    models_.at(region).dirty = false;
}

std::vector<int> MechanicsSourceManager::regions() const // 返回当前管理器中已经配置模型的所有区域编号
{
    std::vector<int> result;
    for (const auto& [region, assignment] : models_) 
    { 
        result.push_back(region); 
    }
    return result;
}

}
