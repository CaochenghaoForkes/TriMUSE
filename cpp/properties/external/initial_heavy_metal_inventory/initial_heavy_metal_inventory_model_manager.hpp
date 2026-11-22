#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class InitialHeavyMetalInventoryModel
{
public:
    virtual ~InitialHeavyMetalInventoryModel();
    virtual StateValue evaluate(const PropertiesStatePoint& point) const = 0;
};

class InitialHeavyMetalInventoryModelManager
{
public:
    void register_models();
    void assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const;
    const std::vector<RequiredState>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<InitialHeavyMetalInventoryModel> registry_;
    std::unordered_map<int, std::unique_ptr<InitialHeavyMetalInventoryModel>> region_model_map_;
};

}
