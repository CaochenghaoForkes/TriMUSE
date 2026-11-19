#pragma once

#include <map>

#include "common/model_registry.hpp"
#include "fem/fem_system.hpp"
#include "physics/mechanics/mechanics_domain_config.hpp"
#include "physics/mechanics/thermal_expansion/mechanics_thermal_expansion_model.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

class MechanicsThermalExpansionManager // 按区域管理模型，缓存全域积分点上的热膨胀载荷系数 beta及组装变化标记
{
public:
    explicit MechanicsThermalExpansionManager(const FemSystem& fem, const std::vector<MechanicsDomainConfig>& configs); // 注册与安排各个热膨胀模型

    void update(const PropertiesFieldData& properties); // 更新热膨胀载荷向量以及对应的 dirty

    bool coupled(const PropertiesSystem& properties, const std::string& field) const; // 判断: 当前管理器负责的模型中，是否有任何一个模型的依赖属性与指定求解场耦合

    void mark_coupled(PropertiesSystem& properties, const std::string& field) const; // 把当前管理器所依赖的属性，交给属性系统登记与指定场的耦合关系

    bool required_no_change(int region) const; // 根据 dirty 判断指定区域的系数是否与最近一次组装时一致(dirty是否为False)，因而可以复用已有矩阵或向量

    void assembled(int region); // 将 region 处的 dirty 记为 false

    const std::vector<VectorValue>& values() const { return values_; } // 取得全求积点向量

    std::vector<int> regions() const; // 返回当前管理器中已经配置模型的所有区域编号

private:
    struct Assignment
    {
        std::unique_ptr<MechanicsThermalExpansionModel> model;
        std::vector<std::string> required;
        bool dirty = true; // 首次必须组装，之后保持变化标记直到 assembled() 清除
    };

    const FemSystem& fem_;
    ModelRegistry<MechanicsThermalExpansionModel> registry_;
    std::map<int, Assignment> models_;
    std::vector<VectorValue> values_; // 下标沿用原网格的全域积分点编号
};

}
