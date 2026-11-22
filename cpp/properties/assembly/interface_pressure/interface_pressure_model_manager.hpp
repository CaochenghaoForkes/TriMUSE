#pragma once

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class InterfacePressureProperty // 属性模型只计算压力，受压侧由物理模型处理
{
public:
    virtual ~InterfacePressureProperty() = default;
    virtual double evaluate(const PropertiesStatePoint& point) const = 0;
};

class InterfacePressureModelManager
{
public:
    void register_models();
    void assign(int attribute, const std::string& model, const ModelParameterMap& parameters);
    const std::vector<RequiredState>& required_states(const std::string& model) const;
    double evaluate(const PropertiesStatePoint& point) const;

private:
    ModelRegistry<InterfacePressureProperty> registry_;
    std::unordered_map<int, std::unique_ptr<InterfacePressureProperty>> interface_models_;
};

}
