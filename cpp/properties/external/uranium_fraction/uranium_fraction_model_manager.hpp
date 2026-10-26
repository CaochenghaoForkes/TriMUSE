#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class UraniumFractionModel
{
public:
    virtual ~UraniumFractionModel() = default;

    virtual StateValue evaluate(const PropertiesStatePoint& point) const = 0;
};

class UraniumFractionModelManager
{
public:
    void register_models();

    void assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const;

    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<UraniumFractionModel, const std::string&> registry_;
    std::unordered_map<int, std::unique_ptr<UraniumFractionModel>> region_model_map_;
};

}
