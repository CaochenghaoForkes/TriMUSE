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
}

void MaterialModelManager::assign(int region_attribute, const std::string& property_name, const std::string& model_name, const ModelParameterMap& parameters)
{
    const auto separator = property_name.find('.');
    const std::string physics_name = property_name.substr(0, separator);
    const std::string property = property_name.substr(separator + 1);
    if (physics_name == "general")
    {
        general_model_manager_.assign(region_attribute, property, model_name, parameters);
        return;
    }
    if (physics_name == "thermal")
    {
        thermal_model_manager_.assign(region_attribute, property, model_name, parameters);
        return;
    }
    throw std::out_of_range("尚未支持的物性分类: " + physics_name);
}

StateValue MaterialModelManager::evaluate(const std::string& property_name, const PropertiesStatePoint& state_point) const
{
    const auto separator = property_name.find('.');
    const std::string physics_name = property_name.substr(0, separator);
    const std::string property = property_name.substr(separator + 1);
    if (physics_name == "general")
    {
        return general_model_manager_.evaluate(property, state_point);
    }
    if (physics_name == "thermal")
    {
        return thermal_model_manager_.evaluate(property, state_point);
    }
    throw std::out_of_range("尚未支持的物性分类: " + physics_name);
}

const std::vector<std::string>& MaterialModelManager::required_states(const std::string& property_name, const std::string& model_name) const
{
    const auto separator = property_name.find('.');
    const std::string physics_name = property_name.substr(0, separator);
    const std::string property = property_name.substr(separator + 1);
    if (physics_name == "general")
    {
        return general_model_manager_.required_states(property, model_name);
    }
    if (physics_name == "thermal")
    {
        return thermal_model_manager_.required_states(property, model_name);
    }
    throw std::out_of_range("尚未支持的物性分类: " + physics_name);
}

}
