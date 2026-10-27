#include "physics/thermal/boundary/thermal_boundary_manager.hpp"
#include "physics/thermal/boundary/thermal_dirichlet_boundary.hpp"
#include "physics/thermal/boundary/thermal_neumann_boundary.hpp"
#include "physics/thermal/boundary/thermal_robin_boundary.hpp"

namespace trimuse
{

ThermalBoundaryManager::ThermalBoundaryManager(const std::vector<ThermalBoundaryConfig>& configs) // 注册热边界模型 并为各个边界配置具体模型
{
    registry_.register_model<ThermalDirichletBoundary>("dirichlet", {"external.surface_temperature"});
    registry_.register_model<ThermalNeumannBoundary>("neumann", {"external.heat_flux"});
    registry_.register_model<ThermalRobinBoundary>("robin", {"derived.heat_transfer_coef", "external.environment_temperature"});
    for (const auto& config : configs) // 逐条读取边界配置
    {
        const auto& entry = registry_.registration(config.model_name); // 取得指定模型的注册记录 entry.factory(工厂函数) entry.required_states(依赖属性)
        models_.emplace(config.boundary_attribute, Assignment{entry.factory(config.parameters), entry.required_states}); // 边界区域编号 -> 该区域的模型信息{具体模型,依赖参数} // 41 → Assignment{Robin 模型对象,{换热系数属性名, 环境温度属性名},{0, 0, 0},{true, true, true}}
    }
}

void ThermalBoundaryManager::update(const PropertiesFieldData& properties) // 更新各边界模型的返回值，并分别标记矩阵项、右端项和指定温度是否需要更新
{
    for (auto& [zone, assignment] : models_) // 遍历各个边界区域的编号与对应模型信息
    {
        const auto value = assignment.model->evaluate(properties, zone); // 根据传入的属性与各个区域的具体模型更新边界返回值 {matrix,load,temperature}
        assignment.dirty[0] = assignment.dirty[0] || value.matrix != assignment.values.matrix; // 矩阵系数发生变化，或者此前尚未完成组装，就保持 dirty[0]=true
        assignment.dirty[1] = assignment.dirty[1] || value.load != assignment.values.load; // 右端项同理
        assignment.dirty[2] = assignment.dirty[2] || value.temperature != assignment.values.temperature; // 同样判断指定表面温度
        assignment.values = value; // 把新返回值保存下来，供组装层读取，也作为下次比较的基准
    }
}

void ThermalBoundaryManager::mark_coupled(PropertiesSystem& properties, const std::string& field) const // 向属性系统登记: 边界模型所依赖的属性，哪些需要随指定求解场更新
{
    for (const auto& [zone, assignment] : models_) // 遍历各边界分区及其模型记录
    { 
        properties.mark_coupled(field, assignment.required); 
    }
}

bool ThermalBoundaryManager::coupled(const PropertiesSystem& properties, const std::string& field) const // 判断场与依赖属性是否存在耦合关系
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

bool ThermalBoundaryManager::required_no_change(int zone, BoundaryPart part) const // 判断指定边界分区的某一部分是否可以复用
{
    return !models_.at(zone).dirty[static_cast<int>(part)]; // 找到编号为 zone 的边界模型记录 -> static_cast<int>(part) 将枚举 BoundaryPart 转为数组下标 -> 查看该部分是否发生变化
}

void ThermalBoundaryManager::assembled(int zone, BoundaryPart part) // 标记指定边界分区的指定部分已被组装更新
{
    models_.at(zone).dirty[static_cast<int>(part)] = false;
}

std::vector<int> ThermalBoundaryManager::zones() const // 返回当前边界管理器中，已配置边界模型的所有 zone 编号
{
    std::vector<int> result;
    for (const auto& [zone, assignment] : models_) 
    { 
        result.push_back(zone); 
    }
    return result;
}

const ThermalBoundaryValues& ThermalBoundaryManager::values(int zone) const // 返回指定区域的 values.matrix: 边界矩阵系数 values.load 边界右端系数 values.temperature 指定表面温度
{ 
    return models_.at(zone).values; 
}

bool ThermalBoundaryManager::essential(int zone) const // 判断指定区域是否是一类边界条件需要约束
{ 
    return models_.at(zone).model->essential(); 
}

}
