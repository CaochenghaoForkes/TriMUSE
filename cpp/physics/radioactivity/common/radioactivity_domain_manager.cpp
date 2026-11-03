#include "physics/radioactivity/common/radioactivity_domain_manager.hpp"
namespace trimuse
{

RadioactivityDomainManager::RadioactivityDomainManager(const FemSystem& fem) // 系数数组按全域求积点总数分配
    : fem_(fem), values_(fem.domain_quadrature_point_count(), 0.0)
{
}

std::vector<RadioactivityDomainConfig> RadioactivityDomainManager::defaults(const FemSystem& fem, const std::string& model) // 为所有材料区域安排指定模型和空参数表
{
    std::vector<RadioactivityDomainConfig> configs;
    for (int region : fem.mesh_data().region_attributes()) { configs.push_back({region, model, {}}); }

    return configs;
}

void RadioactivityDomainManager::assign(const std::vector<RadioactivityDomainConfig>& configs) // 注册表工厂创建具体模型，按区域保存
{
    for (const auto& config : configs)
    {
        const auto& entry = registry_.registration(config.model_name);
        models_.emplace(config.region_attribute, Assignment{entry.factory(config.parameters), entry.required_states});
    }
}

void RadioactivityDomainManager::update(const PropertiesFieldData& properties) // 重新求系数并与已缓存积分点值比较，累计区域重组标记
{
    for (auto& [region, assignment] : models_)
    for (int element : fem_.mesh_data().region_elements(region))
    for (std::size_t point = fem_.domain_quadrature_offset(element);
         point < fem_.domain_quadrature_offset(element + 1); ++point)
    {
        const double value = assignment.model->evaluate(properties, point);
        assignment.dirty = assignment.dirty || value != values_[point]; // 任一积分点变化则整区域需要重组
        values_[point] = value; // 下标为原网格单元偏移 + 局部积分点编号
    }
}

bool RadioactivityDomainManager::coupled(const PropertiesSystem& properties, const std::string& field) const // 任一所需属性参与指定场耦合即返回 true
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

void RadioactivityDomainManager::mark_coupled(PropertiesSystem& properties, const std::string& field) const // 由属性系统判断依赖是否实际涉及该场
{
    for (const auto& [region, assignment] : models_)
    {
        properties.mark_coupled(field, assignment.required);
    }
}

bool RadioactivityDomainManager::required_no_change(int region) const // dirty 为 false 时可复用该区域的矩阵或右端
{
    return !models_.at(region).dirty;
}

void RadioactivityDomainManager::assembled(int region) // 组装完成后清除变化标记
{
    models_.at(region).dirty = false;
}

std::vector<int> RadioactivityDomainManager::regions() const // 按 models_ 的区域键返回已分配模型的区域
{
    std::vector<int> result;
    for (const auto& [region, assignment] : models_)
    {
        result.push_back(region);
    }
    return result;
}

}
