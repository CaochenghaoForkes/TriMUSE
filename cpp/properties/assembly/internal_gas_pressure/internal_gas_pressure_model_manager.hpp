#pragma once

#include "common/model_registry.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class InternalGasPressureModel
{
public:
    virtual ~InternalGasPressureModel();
    virtual double evaluate(const PropertiesStatePoint& point) const = 0;
};

class InternalGasPressureModelManager
{
public:
    void register_models();
    void assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& point) const;
    const std::vector<RequiredState>& required_states(const std::string& model_name) const;

private:
    ModelRegistry<InternalGasPressureModel> registry_;
    std::unordered_map<int, std::unique_ptr<InternalGasPressureModel>> region_model_map_;
};

}
