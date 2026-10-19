#include "properties/material/general/general_model_manager.hpp"

#include <stdexcept>

namespace trimuse
{

void GeneralModelManager::register_models()
{
    porosity_model_manager_.register_models();
    mass_density_model_manager_.register_models();
}

void GeneralModelManager::assign(int region_attribute, const std::string& property_name, const std::string& model_name, const ModelParameterMap& parameters)
{
    if (property_name == "porosity")
    {
        porosity_model_manager_.assign(region_attribute, model_name, parameters);
        return;
    }
    if (property_name == "mass_density")
    {
        mass_density_model_manager_.assign(region_attribute, model_name, parameters);
        return;
    }
    throw std::out_of_range("尚未支持的通用物性: " + property_name);
}

StateValue GeneralModelManager::evaluate(const std::string& property_name, const PropertiesStatePoint& state_point) const
{
    if (property_name == "porosity")
    {
        return porosity_model_manager_.evaluate(state_point);
    }
    if (property_name == "mass_density")
    {
        return mass_density_model_manager_.evaluate(state_point);
    }
    throw std::out_of_range("尚未支持的通用物性: " + property_name);
}

const std::vector<std::string>& GeneralModelManager::required_states(const std::string& property_name, const std::string& model_name) const
{
    if (property_name == "porosity")
    {
        return porosity_model_manager_.required_states(model_name);
    }
    if (property_name == "mass_density")
    {
        return mass_density_model_manager_.required_states(model_name);
    }
    throw std::out_of_range("尚未支持的通用物性: " + property_name);
}

}
