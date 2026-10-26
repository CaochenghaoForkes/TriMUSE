#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class BurnupFimaFractionModel
{
public:
    virtual ~BurnupFimaFractionModel() = default;

    virtual StateValue evaluate(const PropertiesStatePoint& point) const = 0;
};

class BurnupFimaFractionModelManager
{
public:
    void register_models();

    void assign(const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const;

    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<BurnupFimaFractionModel, const std::string&> registry_;
    std::unique_ptr<BurnupFimaFractionModel> model_;
};

}
