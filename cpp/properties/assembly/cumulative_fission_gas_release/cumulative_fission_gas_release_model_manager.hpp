#pragma once

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class CumulativeFissionGasReleaseModel
{
public:
    virtual ~CumulativeFissionGasReleaseModel();
    virtual double evaluate(const PropertiesStatePoint& point) const = 0;
};

class CumulativeFissionGasReleaseModelManager
{
public:
    void register_models();
    void assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& point) const;
    const std::vector<RequiredState>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<CumulativeFissionGasReleaseModel> registry_;
    std::unordered_map<int, std::unique_ptr<CumulativeFissionGasReleaseModel>> region_model_map_;
};

}
