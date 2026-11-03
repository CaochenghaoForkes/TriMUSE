#pragma once

#include <array>
#include <map>
#include "common/model_registry.hpp"
#include "physics/thermal/boundary/thermal_boundary_model.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

class ThermalBoundaryManager
{
public:
    explicit ThermalBoundaryManager(const std::vector<ThermalBoundaryConfig>& configs); // 注册热边界模型 并为各个边界配置具体模型

    void update(const PropertiesFieldData& properties); // 更新各边界模型的返回值，并分别标记矩阵项、右端项和指定温度是否需要更新

    void mark_coupled(PropertiesSystem& properties, const std::string& field) const; // 向属性系统登记: 边界模型所依赖的属性，哪些需要随指定求解场更新

    bool coupled(const PropertiesSystem& properties, const std::string& field) const; // 判断场与依赖属性是否存在耦合关系

    bool required_no_change(int zone, BoundaryPart part) const; // 判断指定边界分区的某一部分是否可以复用

    void assembled(int zone, BoundaryPart part); // 标记指定边界分区的指定部分已被组装更新

    std::vector<int> zones() const; // 返回当前边界管理器中，已配置边界模型的所有 zone 编号

    const ThermalBoundaryValues& values(int zone) const; // 返回指定区域的 values.matrix: 边界矩阵系数 values.load 边界右端系数 values.temperature 指定表面温度

    bool essential(int zone) const; // 判断指定区域是否是一类边界条件需要约束

private:
    struct Assignment // 保存 model 某个边界分区的模型、required 依赖属性、values 最新求值结果（默认都是0），dirty 以及组装变化标记（默认都是true）
    {
        std::unique_ptr<ThermalBoundaryModel> model; // 独占拥有该分区的具体边界模型对象，例如 ThermalRobinBoundary

        std::vector<std::string> required; // 保存模型依赖的属性名称

        ThermalBoundaryValues values; // values.matrix: 边界矩阵系数 values.load 边界右端系数 values.temperature 指定表面温度 默认都是 0

        std::array<bool, 3> dirty{true, true, true}; // 分别记录这三部分是否需要处理重组
    };

    ModelRegistry<ThermalBoundaryModel> registry_; // 模型名 -> 注册记录{工厂函数、依赖属性名称列表}

    std::map<int, Assignment> models_; // 保存各边界分区的实际分配 如 边界 41 → Assignment{Robin 模型对象、依赖属性、缓存值、dirty}
};

}
