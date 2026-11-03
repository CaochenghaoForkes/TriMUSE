#include "properties/external/external_model_manager.hpp"

#include <stdexcept>

namespace trimuse
{

ExternalModelManager::ExternalModelManager()
{
    register_models();
}

void ExternalModelManager::register_models()
{
    power_model_manager_.register_models();
    environment_temperature_model_manager_.register_models();
    environment_nuclide_concentration_model_manager_.register_models();
    surface_temperature_model_manager_.register_models();
    surface_nuclide_concentration_model_manager_.register_models();
    heat_flux_model_manager_.register_models();
    nuclide_flux_model_manager_.register_models();
    burnup_fima_fraction_model_manager_.register_models();
    uranium_fraction_model_manager_.register_models();
    nuclide_birth_rate_model_manager_.register_models();
    fast_neutron_flux_model_manager_.register_models();
}

void ExternalModelManager::assign(const std::string& name, int attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    if (name == "external.power")
    {
        power_model_manager_.assign(attribute, model_name, parameters, value_type);
        return;
    }
    if (name == "external.environment_temperature")
    {
        environment_temperature_model_manager_.assign(attribute, model_name, parameters, value_type);
        return;
    }
    if (name == "external.heat_flux")
    {
        heat_flux_model_manager_.assign(attribute, model_name, parameters, value_type);
        return;
    }
    if (name == "external.surface_temperature")
    {
        surface_temperature_model_manager_.assign(attribute, model_name, parameters, value_type);
        return;
    }
    if (name == "external.burnup_fima_fraction")
    {
        burnup_fima_fraction_model_manager_.assign(model_name, parameters, value_type);
        return;
    }
    if (name == "external.uranium_fraction")
    {
        uranium_fraction_model_manager_.assign(attribute, model_name, parameters, value_type);
        return;
    }
    if (name == "external.nuclide_birth_rate")
    {
        nuclide_birth_rate_model_manager_.assign(model_name, parameters, value_type);
        return;
    }
    if (name == "external.fast_neutron_flux")
    {
        fast_neutron_flux_model_manager_.assign(model_name, parameters, value_type);
        return;
    }
    if (name == "external.environment_nuclide_concentration")
    {
        environment_nuclide_concentration_model_manager_.assign(attribute, model_name, parameters, value_type);
        return;
    }
    if (name == "external.surface_nuclide_concentration")
    {
        surface_nuclide_concentration_model_manager_.assign(attribute, model_name, parameters, value_type);
        return;
    }
    if (name == "external.nuclide_flux")
    {
        nuclide_flux_model_manager_.assign(attribute, model_name, parameters, value_type);
        return;
    }
    throw std::out_of_range("尚未支持的外部量: " + name);
}

StateValue ExternalModelManager::evaluate(const std::string& name, const PropertiesStatePoint& point) const
{
    if (name == "external.power")
    {
        return power_model_manager_.evaluate(point);
    }
    if (name == "external.environment_temperature")
    {
        return environment_temperature_model_manager_.evaluate(point);
    }
    if (name == "external.heat_flux")
    {
        return heat_flux_model_manager_.evaluate(point);
    }
    if (name == "external.surface_temperature")
    {
        return surface_temperature_model_manager_.evaluate(point);
    }
    if (name == "external.burnup_fima_fraction")
    {
        return burnup_fima_fraction_model_manager_.evaluate(point);
    }
    if (name == "external.uranium_fraction")
    {
        return uranium_fraction_model_manager_.evaluate(point);
    }
    if (name == "external.nuclide_birth_rate")
    {
        return nuclide_birth_rate_model_manager_.evaluate(point);
    }
    if (name == "external.fast_neutron_flux")
    {
        return fast_neutron_flux_model_manager_.evaluate(point);
    }
    if (name == "external.environment_nuclide_concentration")
    {
        return environment_nuclide_concentration_model_manager_.evaluate(point);
    }
    if (name == "external.surface_nuclide_concentration")
    {
        return surface_nuclide_concentration_model_manager_.evaluate(point);
    }
    if (name == "external.nuclide_flux")
    {
        return nuclide_flux_model_manager_.evaluate(point);
    }
    throw std::out_of_range("尚未支持的外部量: " + name);
}

const std::vector<std::string>& ExternalModelManager::required_states(const std::string& name, const std::string& model_name) const
{
    if (name == "external.power")
    {
        return power_model_manager_.required_states(model_name);
    }
    if (name == "external.environment_temperature")
    {
        return environment_temperature_model_manager_.required_states(model_name);
    }
    if (name == "external.heat_flux")
    {
        return heat_flux_model_manager_.required_states(model_name);
    }
    if (name == "external.surface_temperature")
    {
        return surface_temperature_model_manager_.required_states(model_name);
    }
    if (name == "external.burnup_fima_fraction")
    {
        return burnup_fima_fraction_model_manager_.required_states(model_name);
    }
    if (name == "external.uranium_fraction")
    {
        return uranium_fraction_model_manager_.required_states(model_name);
    }
    if (name == "external.nuclide_birth_rate")
    {
        return nuclide_birth_rate_model_manager_.required_states(model_name);
    }
    if (name == "external.fast_neutron_flux")
    {
        return fast_neutron_flux_model_manager_.required_states(model_name);
    }
    if (name == "external.environment_nuclide_concentration")
    {
        return environment_nuclide_concentration_model_manager_.required_states(model_name);
    }
    if (name == "external.surface_nuclide_concentration")
    {
        return surface_nuclide_concentration_model_manager_.required_states(model_name);
    }
    if (name == "external.nuclide_flux")
    {
        return nuclide_flux_model_manager_.required_states(model_name);
    }
    throw std::out_of_range("尚未支持的外部量: " + name);
}

}
