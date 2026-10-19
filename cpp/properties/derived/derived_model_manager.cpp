#include "properties/derived/derived_model_manager.hpp"

#include <stdexcept>

namespace trimuse
{

DerivedModelManager::DerivedModelManager()
{
    register_models();
}

void DerivedModelManager::register_models()
{
    volumetric_power_model_manager_.register_models();
    heat_transfer_coef_model_manager_.register_models();
}

void DerivedModelManager::assign(const std::string& name, int attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    if (name == "volumetric_power")
    {
        volumetric_power_model_manager_.assign(attribute, model_name, parameters);
        return;
    }
    if (name == "heat_transfer_coef")
    {
        heat_transfer_coef_model_manager_.assign(attribute, model_name, parameters);
        return;
    }
    throw std::out_of_range("尚未支持的派生量: " + name);
}

StateValue DerivedModelManager::evaluate(const std::string& name, const PropertiesStatePoint& point) const
{
    if (name == "volumetric_power")
    {
        return volumetric_power_model_manager_.evaluate(point);
    }
    if (name == "heat_transfer_coef")
    {
        return heat_transfer_coef_model_manager_.evaluate(point);
    }
    throw std::out_of_range("尚未支持的派生量: " + name);
}

const std::vector<std::string>& DerivedModelManager::required_states(const std::string& name, const std::string& model_name) const
{
    if (name == "volumetric_power")
    {
        return volumetric_power_model_manager_.required_states(model_name);
    }
    if (name == "heat_transfer_coef")
    {
        return heat_transfer_coef_model_manager_.required_states(model_name);
    }
    throw std::out_of_range("尚未支持的派生量: " + name);
}

}
