#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class InterfaceAreaModel
{
public:
    virtual ~InterfaceAreaModel();
    virtual StateValue evaluate(const PropertiesStatePoint& point) const = 0;
};

class InterfaceAreaModelManager
{
public:
    void register_models();
    void assign(int interface_attribute, const std::string& model_name, const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const;
    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<InterfaceAreaModel> registry_;
    std::unordered_map<int, std::unique_ptr<InterfaceAreaModel>> interface_model_map_;
};

}
