#include "properties/material/mechanics/mechanics_model_manager.hpp"

#include <stdexcept>

namespace trimuse
{

void MechanicsModelManager::register_models()
{
    young_modulus_model_manager_.register_models();
    poisson_ratio_model_manager_.register_models();
    lame_lambda_model_manager_.register_models();
    shear_modulus_model_manager_.register_models();
}

void MechanicsModelManager::assign(int region_attribute, const std::string& property_name, const std::string& model_name, const ModelParameterMap& parameters)
{
    if (property_name == "material.mechanics.young_modulus")
    {
        young_modulus_model_manager_.assign(region_attribute, model_name, parameters);
        return;
    }
    if (property_name == "material.mechanics.poisson_ratio")
    {
        poisson_ratio_model_manager_.assign(region_attribute, model_name, parameters);
        return;
    }
    if (property_name == "material.mechanics.lame_lambda")
    {
        lame_lambda_model_manager_.assign(region_attribute, model_name, parameters);
        return;
    }
    if (property_name == "material.mechanics.shear_modulus")
    {
        shear_modulus_model_manager_.assign(region_attribute, model_name, parameters);
        return;
    }
    throw std::out_of_range("尚未支持的力学物性: " + property_name);
}

StateValue MechanicsModelManager::evaluate(const std::string& property_name, const PropertiesStatePoint& state_point) const
{
    if (property_name == "material.mechanics.young_modulus")
    {
        return young_modulus_model_manager_.evaluate(state_point);
    }
    if (property_name == "material.mechanics.poisson_ratio")
    {
        return poisson_ratio_model_manager_.evaluate(state_point);
    }
    if (property_name == "material.mechanics.lame_lambda")
    {
        return lame_lambda_model_manager_.evaluate(state_point);
    }
    if (property_name == "material.mechanics.shear_modulus")
    {
        return shear_modulus_model_manager_.evaluate(state_point);
    }
    throw std::out_of_range("尚未支持的力学物性: " + property_name);
}

const std::vector<std::string>& MechanicsModelManager::required_states(const std::string& property_name, const std::string& model_name) const
{
    if (property_name == "material.mechanics.young_modulus")
    {
        return young_modulus_model_manager_.required_states(model_name);
    }
    if (property_name == "material.mechanics.poisson_ratio")
    {
        return poisson_ratio_model_manager_.required_states(model_name);
    }
    if (property_name == "material.mechanics.lame_lambda")
    {
        return lame_lambda_model_manager_.required_states(model_name);
    }
    if (property_name == "material.mechanics.shear_modulus")
    {
        return shear_modulus_model_manager_.required_states(model_name);
    }
    throw std::out_of_range("尚未支持的力学物性: " + property_name);
}

}
