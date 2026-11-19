#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class BurnupFimaFractionModel
{
public:
    virtual ~BurnupFimaFractionModel();
    virtual StateValue evaluate(const PropertiesStatePoint& point) const = 0;
};

class BurnupFimaFractionModelManager
{
public:
    void register_models();
    void assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const;
    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<BurnupFimaFractionModel> registry_;
    std::unordered_map<int, std::unique_ptr<BurnupFimaFractionModel>> region_model_map_;
};

}
