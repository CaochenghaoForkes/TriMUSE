#include "properties/material/material_model_manager.hpp"

#include <stdexcept>

namespace trimuse
{

MaterialModelManager::MaterialModelManager()
{
    register_models();
}

void MaterialModelManager::register_models()
{
    general_model_manager_.register_models();
    thermal_model_manager_.register_models();
    mechanics_model_manager_.register_models();
    radioactivity_model_manager_.register_models();
}

void MaterialModelManager::assign(int region_attribute, const std::string& property_name, const std::string& model_name, const ModelParameterMap& parameters)
{
    if (property_name.rfind("material.general.", 0) == 0)
    {
        general_model_manager_.assign(region_attribute, property_name, model_name, parameters);
        return;
    }
    if (property_name.rfind("material.thermal.", 0) == 0)
    {
        thermal_model_manager_.assign(region_attribute, property_name, model_name, parameters);
        return;
    }
    if (property_name.rfind("material.mechanics.", 0) == 0)
    {
        mechanics_model_manager_.assign(region_attribute, property_name, model_name, parameters);
        return;
    }
    if (property_name.rfind("material.radioactivity.", 0) == 0)
    {
        radioactivity_model_manager_.assign(region_attribute, property_name, model_name, parameters);
        return;
    }
    throw std::out_of_range("尚未支持的材料属性: " + property_name);
}

StateValue MaterialModelManager::evaluate(const std::string& property_name, const PropertiesStatePoint& state_point) const
{
    if (property_name.rfind("material.general.", 0) == 0)
    {
        return general_model_manager_.evaluate(property_name, state_point);
    }
    if (property_name.rfind("material.thermal.", 0) == 0)
    {
        return thermal_model_manager_.evaluate(property_name, state_point);
    }
    if (property_name.rfind("material.mechanics.", 0) == 0)
    {
        return mechanics_model_manager_.evaluate(property_name, state_point);
    }
    if (property_name.rfind("material.radioactivity.", 0) == 0)
    {
        return radioactivity_model_manager_.evaluate(property_name, state_point);
    }
    throw std::out_of_range("尚未支持的材料属性: " + property_name);
}

const std::vector<std::string>& MaterialModelManager::required_states(const std::string& property_name, const std::string& model_name) const
{
    if (property_name.rfind("material.general.", 0) == 0)
    {
        return general_model_manager_.required_states(property_name, model_name);
    }
    if (property_name.rfind("material.thermal.", 0) == 0)
    {
        return thermal_model_manager_.required_states(property_name, model_name);
    }
    if (property_name.rfind("material.mechanics.", 0) == 0)
    {
        return mechanics_model_manager_.required_states(property_name, model_name);
    }
    if (property_name.rfind("material.radioactivity.", 0) == 0)
    {
        return radioactivity_model_manager_.required_states(property_name, model_name);
    }
    throw std::out_of_range("尚未支持的材料属性: " + property_name);
}

}
