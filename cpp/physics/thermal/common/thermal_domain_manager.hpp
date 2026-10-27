#pragma once

#include <map>
#include "common/model_registry.hpp"
#include "fem/fem_system.hpp"
#include "physics/thermal/common/thermal_domain_model.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

class ThermalDomainManager // 域内热物理管理器的公共基类
{

public:
    void update(const PropertiesFieldData& properties); // 重新计算各区域积分点上的系数，并判断它们是否发生变化，需要重新组装

    bool coupled(const PropertiesSystem& properties, const std::string& field) const; // 判断: 当前管理器负责的模型中，是否有任何一个与指定求解场耦合

    void mark_coupled(PropertiesSystem& properties, const std::string& field) const; // 把当前管理器所依赖的属性，交给属性系统登记与指定场的耦合关系

    bool required_no_change(int region) const; // 判断: 指定区域的系数是否与最近一次组装时一致，因而可以复用已有矩阵或向量

    void assembled(int region); // 装配后将 region 处的 dirty 记为 false

    const std::vector<double>& values() const { return values_; } // 返回保存该管理器算出的全域积分点系数
    
    std::vector<int> regions() const; // 返回当前管理器中已经配置模型的所有区域编号

protected:
    explicit ThermalDomainManager(const FemSystem& fem); // 初始化 values 的长度为全域积分点总数 值为 0.0 models_ 初始化为空

    void assign(const std::vector<ThermalDomainConfig>& configs); // 根据配置列表 为每个区域创建具体模型 并保存到 models_ 中

    static std::vector<ThermalDomainConfig> defaults(const FemSystem& fem, const std::string& model); // 用于给网格中的每个区域生成相同的默认模型配置

    ModelRegistry<ThermalDomainModel> registry_; // 注册表

private:
    struct Assignment // 某个区域已经分配好的模型，以及该模型的依赖和组装状态
    {
        std::unique_ptr<ThermalDomainModel> model; // 独占拥有一个具体模型对象 如 FourierConduction 通过基类指针统一管理不同模型，并调用它们的 evaluate()
        std::vector<std::string> required; // 该模型依赖的属性名称 如 {"material.thermal.conductivity"}
        bool dirty = true; // 表示该区域的系数是否需要重新组装到矩阵或向量中 初始为: true 尚未组装，需要首次组装 update() 发现系数变化: 设为 true 组装完后调用 assembled(): 设为 false 系数未变且已组装: 保存 false 复用缓存
    };

    const FemSystem& fem_; // 保存有限元系统

    std::map<int, Assignment> models_; // 区域编号 -> 模型及其运行状态 如 区域 3  -> Assignment {模型对象、依赖属性列表、dirty}

    std::vector<double> values_; // 保存该管理器算出的全域积分点系数，下标为 单元的积分点偏移 + 单元内积分点编号 如 k、rho*cp、q update() 根据 models_ 中的模型计算系数，写入 values_；组装层再读取这些系数完成单元积分
};
}
