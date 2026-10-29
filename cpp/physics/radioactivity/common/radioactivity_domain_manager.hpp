#pragma once

#include <map>
#include "common/model_registry.hpp"
#include "fem/fem_system.hpp"
#include "physics/radioactivity/common/radioactivity_domain_model.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

class RadioactivityDomainManager
{
public:
    void update(const PropertiesFieldData& properties);

    bool coupled(const PropertiesSystem& properties, const std::string& field) const;

    void mark_coupled(PropertiesSystem& properties, const std::string& field) const;

    bool required_no_change(int region) const; // 与最近一次组装的系数一致，可以复用

    void assembled(int region);

    const std::vector<double>& values() const { return values_; }

    std::vector<int> regions() const;

protected:
    explicit RadioactivityDomainManager(const FemSystem& fem);

    void assign(const std::vector<RadioactivityDomainConfig>& configs);

    static std::vector<RadioactivityDomainConfig> defaults(const FemSystem& fem, const std::string& model);

    ModelRegistry<RadioactivityDomainModel> registry_;

private:
    struct Assignment
    {
        std::unique_ptr<RadioactivityDomainModel> model;
        std::vector<std::string> required;
        bool dirty = true; // 首次或系数改变后置 true，assembled() 后才清除
    };

    const FemSystem& fem_;

    std::map<int, Assignment> models_;

    std::vector<double> values_; // 原网格全域积分点编号 -> 当前模型系数
};
}
