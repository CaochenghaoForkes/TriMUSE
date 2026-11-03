#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class FastNeutronFluenceModel
{
public:
    virtual ~FastNeutronFluenceModel() = default;

    virtual StateValue evaluate(const PropertiesStatePoint& point) const = 0;
};

class FastNeutronFluenceModelManager
{
public:
    void register_models();

    void assign(const std::string& model_name, const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const;
    
    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<FastNeutronFluenceModel> registry_;
    std::unique_ptr<FastNeutronFluenceModel> model_;
};

}
