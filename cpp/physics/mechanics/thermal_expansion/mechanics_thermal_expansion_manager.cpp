#include "physics/mechanics/thermal_expansion/mechanics_thermal_expansion_manager.hpp"
#include "physics/mechanics/thermal_expansion/linear_thermal_expansion.hpp"

namespace trimuse
{

MechanicsThermalExpansionManager::MechanicsThermalExpansionManager(const FemSystem& fem, const std::vector<MechanicsDomainConfig>& configs) // 注册与安排各个热膨胀模型
    :
    fem_(fem),
    values_(fem.domain_quadrature_point_count(), VectorValue(6, 0.0))
{
    registry_.register_model<LinearThermalExpansion>("linear", {"material.mechanics.elasticity_tensor", "material.mechanics.thermal_expansion_coef", "precomputed.thermal_expansion_reference_temperature", "precomputed.tensor_transform"});
    for (const auto& config : configs)
    {
        const auto& entry = registry_.registration(config.model_name); // 取得各个已安排区域的热膨胀模型的记录
        models_.emplace(config.region_attribute, Assignment{entry.factory(config.parameters), entry.required_states}); // 为各区域安排具体的热膨胀模型
    }
}

void MechanicsThermalExpansionManager::update(const PropertiesFieldData& properties) // 更新热膨胀载荷向量以及对应的 dirty
{
    if (models_.empty())
    {
        return;
    }
    const auto temperature = fem_.field_view("temperature");
    const auto& mesh = fem_.mesh_data().mfem_mesh();
    for (auto& [region, assignment] : models_) // 遍历各个区域的编号与安排好的热碰撞模型
        for (int element : fem_.mesh_data().region_elements(region)) // 遍历当前区域中的各个单元
        {
            const auto* rule = fem_.domain_quadrature_rule(mesh.GetElementBaseGeometry(element)); // 取得域积分规则
            const std::size_t offset = fem_.domain_quadrature_offset(element); // 当前单元求积点在全局存储中的偏移
            for (int q = 0; q < rule->GetNPoints(); ++q) // 遍历该单元求积点
            {
                const double temperature_value = std::get<double>(temperature.evaluate(region, element, rule->IntPoint(q))); // 取该点温度
                VectorValue coefficient = assignment.model->evaluate(properties, offset + q, temperature_value); // 计算该点的热载荷向量
                assignment.dirty = assignment.dirty || coefficient != values_[offset + q]; // 若该区域本就 dirty 或新热膨胀 vector 和 旧热膨胀 vector 不一致，就标记 dirty
                values_[offset + q] = std::move(coefficient);
            }
        }
}

bool MechanicsThermalExpansionManager::coupled(const PropertiesSystem& properties, const std::string& field) const // 判断: 当前管理器负责的模型中，是否有任何一个模型的依赖属性与指定求解场耦合
{
    for (const auto& [region, assignment] : models_) // 遍历各区域及其模型分配记录
        for (const auto& name : assignment.required) // 遍历该模型依赖的属性名称
        {
            if (properties.coupled_with_field(name, region, field))
            {
                return true; // 向属性系统查询这个属性在当前区域是否已标记为与 field 耦合, 若耦合则立刻返回 true
            }
        }
    return false;
}

void MechanicsThermalExpansionManager::mark_coupled(PropertiesSystem& properties, const std::string& field) const // 把当前管理器所依赖的属性，交给属性系统登记与指定场的耦合关系
{
    for (const auto& [region, assignment] : models_)
    {
        properties.mark_coupled(field, assignment.required); // 把指定场名称和模型依赖的属性列表传给属性系统，属性系统会进一步检查：所需要的属性在哪些区域依赖位移，只给符合条件的区域设置耦合标记，并向上游依赖传播标记
    }
}

bool MechanicsThermalExpansionManager::required_no_change(int region) const // 根据 dirty 判断指定区域的系数是否与最近一次组装时一致(dirty是否为False)，因而可以复用已有矩阵或向量
{
    return !models_.at(region).dirty;
}

void MechanicsThermalExpansionManager::assembled(int region) // 将 region 处的 dirty 记为 false
{
    models_.at(region).dirty = false;
}

std::vector<int> MechanicsThermalExpansionManager::regions() const // 返回当前管理器中已经配置模型的所有区域编号
{
    std::vector<int> result;
    for (const auto& [region, assignment] : models_)
    {
        result.push_back(region);
    }
    return result;
}

}
