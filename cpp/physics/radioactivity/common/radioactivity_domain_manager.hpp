#pragma once

#include <map>
#include "common/model_registry.hpp"
#include "fem/fem_system.hpp"
#include "physics/radioactivity/common/radioactivity_domain_model.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

class RadioactivityDomainManager // 扩散、单位容量和体积源共用的区域模型管理逻辑
{
public:
    void update(const PropertiesFieldData& properties); // 更新各区域积分点系数，并标记需要重新组装的区域

    bool coupled(const PropertiesSystem& properties, const std::string& field) const; // 判断所需属性是否与指定求解场耦合

    void mark_coupled(PropertiesSystem& properties, const std::string& field) const; // 将模型依赖交给属性系统登记耦合关系

    bool required_no_change(int region) const; // 与最近一次组装的系数一致，可以复用

    void assembled(int region); // 该区域贡献组装完成后清除 dirty 标记

    const std::vector<double>& values() const { return values_; } // 全域积分点系数数组，供 ScalarIntegrator 读取

    std::vector<int> regions() const; // 返回已配置模型的区域编号

protected:
    explicit RadioactivityDomainManager(const FemSystem& fem);

    void assign(const std::vector<RadioactivityDomainConfig>& configs); // 根据区域配置创建并保存具体模型

    static std::vector<RadioactivityDomainConfig> defaults(const FemSystem& fem, const std::string& model); // 为每个材料区域生成相同的默认模型配置

    ModelRegistry<RadioactivityDomainModel> registry_; // 模型名到工厂和所需属性的注册表

private:
    struct Assignment
    {
        std::unique_ptr<RadioactivityDomainModel> model; // 本区域采用的具体模型
        std::vector<std::string> required; // 模型依赖的属性名
        bool dirty = true; // 首次或系数改变后置 true，assembled() 后才清除
    };

    const FemSystem& fem_; // 提供原网格区域单元表和积分点偏移

    std::map<int, Assignment> models_; // 区域编号 -> 模型及变化状态

    std::vector<double> values_; // 原网格全域积分点编号 -> 当前模型系数
};
}
