#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class NuclideBirthRateModel
{
public:
    virtual ~NuclideBirthRateModel() = default;

    virtual StateValue evaluate(const PropertiesStatePoint& point) const = 0;
};

class NuclideBirthRateModelManager
{
public:
    void register_models();

    void assign(const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const;

    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<NuclideBirthRateModel, const std::string&> registry_;
    std::unique_ptr<NuclideBirthRateModel> model_;
};

}
