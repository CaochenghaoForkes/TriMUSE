#include "properties/material/radioactivity/radioactivity_model_manager.hpp"

#include <stdexcept>

namespace trimuse
{

void RadioactivityModelManager::register_models()
{
    nuclide_diffusion_coef_model_manager_.register_models();
}

void RadioactivityModelManager::assign(int region_attribute, const std::string& property_name, const std::string& model_name, const ModelParameterMap& parameters)
{
    if (property_name == "nuclide_diffusion_coef")
    {
        nuclide_diffusion_coef_model_manager_.assign(region_attribute, model_name, parameters);
        return;
    }
    throw std::out_of_range("尚未支持的放射性物性: " + property_name);
}

StateValue RadioactivityModelManager::evaluate(const std::string& property_name, const PropertiesStatePoint& state_point) const
{
    if (property_name == "nuclide_diffusion_coef")
    {
        return nuclide_diffusion_coef_model_manager_.evaluate(state_point);
    }
    throw std::out_of_range("尚未支持的放射性物性: " + property_name);
}

const std::vector<std::string>& RadioactivityModelManager::required_states(const std::string& property_name, const std::string& model_name) const
{
    if (property_name == "nuclide_diffusion_coef")
    {
        return nuclide_diffusion_coef_model_manager_.required_states(model_name);
    }
    throw std::out_of_range("尚未支持的放射性物性: " + property_name);
}

}
