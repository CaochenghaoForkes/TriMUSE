#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class EnergyPerFissionModel
{
public:
    virtual ~EnergyPerFissionModel();
    virtual StateValue evaluate(const PropertiesStatePoint& point) const = 0;
};

class EnergyPerFissionModelManager
{
public:
    void register_models();
    void assign(const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type);
    StateValue evaluate(const PropertiesStatePoint& point) const;
    const std::vector<RequiredState>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<EnergyPerFissionModel, const std::string&> registry_;
    std::unique_ptr<EnergyPerFissionModel> model_;
};

}
