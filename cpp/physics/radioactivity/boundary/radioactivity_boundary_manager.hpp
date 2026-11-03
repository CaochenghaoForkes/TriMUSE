#pragma once

#include <array>
#include <map>
#include "common/model_registry.hpp"
#include "physics/radioactivity/boundary/radioactivity_boundary_model.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

class RadioactivityBoundaryManager // 按边界分区管理条件，并分别缓存矩阵、载荷和指定浓度系数
{
public:
    explicit RadioactivityBoundaryManager(const std::vector<RadioactivityBoundaryConfig>& configs);

    void update(const PropertiesFieldData& properties); // 根据属性更新各边界模型值及变化标记

    void mark_coupled(PropertiesSystem& properties, const std::string& field) const; // 登记边界模型与指定场的属性依赖

    bool coupled(const PropertiesSystem& properties, const std::string& field) const; // 判断边界所需属性是否与指定场耦合

    bool required_no_change(int zone, RadioactivityBoundaryPart part) const; // 该分区指定部分是否可复用

    void assembled(int zone, RadioactivityBoundaryPart part); // 对应部分处理后清除变化标记

    std::vector<int> zones() const; // 返回已配置的边界属性编号

    const RadioactivityBoundaryValues& values(int zone) const; // 当前边界模型返回的三个系数

    bool essential(int zone) const; // 判断该分区是否为 Dirichlet 边界

private:
    struct Assignment
    {
        std::unique_ptr<RadioactivityBoundaryModel> model; // 本分区采用的具体边界模型

        std::vector<std::string> required; // 模型依赖的属性名

        RadioactivityBoundaryValues values; // 最近一次模型计算的边界系数

        std::array<bool, 3> dirty{true, true, true}; // 按 Matrix、Load、Essential 顺序记录是否需更新
    };

    ModelRegistry<RadioactivityBoundaryModel> registry_; // 模型名 -> 工厂及所需属性

    std::map<int, Assignment> models_; // 边界属性编号 -> 模型、系数与变化状态
};

}
