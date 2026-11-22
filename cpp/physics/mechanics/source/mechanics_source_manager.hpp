#pragma once

#include <map>

#include "common/model_registry.hpp"
#include "fem/fem_system.hpp"
#include "physics/mechanics/source/mechanics_source_model.hpp"
#include "physics/mechanics/mechanics_domain_config.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

class MechanicsSourceManager // 按区域管理体积力模型，缓存全域积分点上的力向量及组装变化标记
{
public:
    explicit MechanicsSourceManager(const FemSystem& fem, const std::vector<MechanicsDomainConfig>& configs = {}); // 只安排显式配置的区域，未配置区域的体积力为零
    void update(const PropertiesFieldData& properties); // 更新各个求积点上的体积力与 dirty 标记
    bool coupled(const PropertiesSystem& properties, const std::string& field) const; // 判断属性是否因体积力而与位移场耦合
    void mark_coupled(PropertiesSystem& properties, const std::string& field) const; // 标记由于体积力而与位移场耦合的属性
    bool required_no_change(int region) const; // 根据 dirty 判断指定区域的系数是否与最近一次组装时一致(dirty是否为False)，因而可以复用已有矩阵或向量
    void assembled(int region); // 将 region 处的 dirty 记为 false
    const std::vector<VectorValue>& values() const { return values_; } // 全域积分点编号 -> 该点的体积力向量
    std::vector<int> regions() const; // 返回当前管理器中已经配置模型的所有区域编号

private:
    struct Assignment
    {
        std::unique_ptr<MechanicsSourceModel> model;
        std::vector<RequiredState> required;
        bool dirty = true; // 首次必须组装，之后保持变化标记直到 assembled() 清除
    };

    const FemSystem& fem_;
    ModelRegistry<MechanicsSourceModel> registry_; // 模型构造时传入参数字典，维数检查由 Python 输入层负责
    std::map<int, Assignment> models_;
    std::vector<VectorValue> values_; // 下标沿用原网格的全域积分点编号
};

}
