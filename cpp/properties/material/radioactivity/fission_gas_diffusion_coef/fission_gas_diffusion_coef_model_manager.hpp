#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class FissionGasDiffusionCoefModel
{
public:
    virtual ~FissionGasDiffusionCoefModel();
    virtual double evaluate(const PropertiesStatePoint& state_point) const = 0;
};

class FissionGasDiffusionCoefModelManager
{
public:
    void register_models();
    void assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& state_point) const;
    const std::vector<RequiredState>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<FissionGasDiffusionCoefModel> registry_;
    std::unordered_map<int, std::unique_ptr<FissionGasDiffusionCoefModel>> region_model_map_;
};

}
