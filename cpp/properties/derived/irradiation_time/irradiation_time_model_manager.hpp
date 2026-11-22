#pragma once

#include <memory>
#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class IrradiationTimeModel
{
public:
    virtual ~IrradiationTimeModel();
    virtual StateValue evaluate(const PropertiesStatePoint& point) const = 0;
};

class IrradiationTimeModelManager
{
public:
    void register_models();
    void assign(const std::string& model_name, const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const;
    const std::vector<RequiredState>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<IrradiationTimeModel> registry_;
    std::unique_ptr<IrradiationTimeModel> model_;
};

}
