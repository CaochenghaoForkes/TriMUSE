#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class LocalBoothFissionGasReleaseStateModel
{
public:
    virtual ~LocalBoothFissionGasReleaseStateModel();
    virtual VectorValue evaluate(const PropertiesStatePoint& state_point) const = 0;
};

class LocalBoothFissionGasReleaseStateModelManager
{
public:
    void register_models();
    void assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters);
    VectorValue evaluate(const PropertiesStatePoint& state_point) const;
    const std::vector<RequiredState>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<LocalBoothFissionGasReleaseStateModel> registry_;
    std::unordered_map<int, std::unique_ptr<LocalBoothFissionGasReleaseStateModel>> region_model_map_;
};

}
