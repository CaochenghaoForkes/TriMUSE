#include "properties/material/thermal/thermal_model_manager.hpp"

#include <stdexcept>

namespace trimuse
{

void ThermalModelManager::register_models()
{
    conductivity_model_manager_.register_models();
    specific_heat_model_manager_.register_models();
}

void ThermalModelManager::assign(int region_attribute, const std::string& property_name, const std::string& model_name, const ModelParameterMap& parameters)
{
    if (property_name == "conductivity")
    {
        conductivity_model_manager_.assign(region_attribute, model_name, parameters);
        return;
    }
    if (property_name == "specific_heat")
    {
        specific_heat_model_manager_.assign(region_attribute, model_name, parameters);
        return;
    }
    throw std::out_of_range("尚未支持的热物性: " + property_name);
}

StateValue ThermalModelManager::evaluate(const std::string& property_name, const PropertiesStatePoint& state_point) const
{
    if (property_name == "conductivity")
    {
        return conductivity_model_manager_.evaluate(state_point);
    }
    if (property_name == "specific_heat")
    {
        return specific_heat_model_manager_.evaluate(state_point);
    }
    throw std::out_of_range("尚未支持的热物性: " + property_name);
}

const std::vector<std::string>& ThermalModelManager::required_states(const std::string& property_name, const std::string& model_name) const
{
    if (property_name == "conductivity")
    {
        return conductivity_model_manager_.required_states(model_name);
    }
    if (property_name == "specific_heat")
    {
        return specific_heat_model_manager_.required_states(model_name);
    }
    throw std::out_of_range("尚未支持的热物性: " + property_name);
}

}
