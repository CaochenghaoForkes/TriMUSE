#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class HomogenizedPanamaOpfModel
{
public:
    virtual ~HomogenizedPanamaOpfModel();
    virtual VectorValue evaluate(const PropertiesStatePoint& state_point) const = 0;
};

class HomogenizedPanamaOpfModelManager
{
public:
    void register_models();
    void assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters);
    VectorValue evaluate(const PropertiesStatePoint& state_point) const;
    const std::vector<RequiredState>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<HomogenizedPanamaOpfModel> registry_;
    std::unordered_map<int, std::unique_ptr<HomogenizedPanamaOpfModel>> region_model_map_;
};

}
