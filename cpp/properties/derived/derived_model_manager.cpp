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
    mass_transfer_coef_model_manager_.register_models();
    volumetric_nuclide_birth_rate_model_manager_.register_models();
    fast_neutron_fluence_model_manager_.register_models();
}

void DerivedModelManager::assign(const std::string& name, int attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    if (name == "derived.volumetric_power")
    {
        volumetric_power_model_manager_.assign(attribute, model_name, parameters);
        return;
    }
    if (name == "derived.heat_transfer_coef")
    {
        heat_transfer_coef_model_manager_.assign(attribute, model_name, parameters);
        return;
    }
    if (name == "derived.volumetric_nuclide_birth_rate")
    {
        volumetric_nuclide_birth_rate_model_manager_.assign(attribute, model_name, parameters);
        return;
    }
    if (name == "derived.mass_transfer_coef")
    {
        mass_transfer_coef_model_manager_.assign(attribute, model_name, parameters);
        return;
    }
    if (name == "derived.fast_neutron_fluence")
    {
        fast_neutron_fluence_model_manager_.assign(model_name, parameters);
        return;
    }
    throw std::out_of_range("尚未支持的派生量: " + name);
}

StateValue DerivedModelManager::evaluate(const std::string& name, const PropertiesStatePoint& point) const
{
    if (name == "derived.volumetric_power")
    {
        return volumetric_power_model_manager_.evaluate(point);
    }
    if (name == "derived.heat_transfer_coef")
    {
        return heat_transfer_coef_model_manager_.evaluate(point);
    }
    if (name == "derived.volumetric_nuclide_birth_rate")
    {
        return volumetric_nuclide_birth_rate_model_manager_.evaluate(point);
    }
    if (name == "derived.mass_transfer_coef")
    {
        return mass_transfer_coef_model_manager_.evaluate(point);
    }
    if (name == "derived.fast_neutron_fluence")
    {
        return fast_neutron_fluence_model_manager_.evaluate(point);
    }
    throw std::out_of_range("尚未支持的派生量: " + name);
}

const std::vector<std::string>& DerivedModelManager::required_states(const std::string& name, const std::string& model_name) const
{
    if (name == "derived.volumetric_power")
    {
        return volumetric_power_model_manager_.required_states(model_name);
    }
    if (name == "derived.heat_transfer_coef")
    {
        return heat_transfer_coef_model_manager_.required_states(model_name);
    }
    if (name == "derived.volumetric_nuclide_birth_rate")
    {
        return volumetric_nuclide_birth_rate_model_manager_.required_states(model_name);
    }
    if (name == "derived.mass_transfer_coef")
    {
        return mass_transfer_coef_model_manager_.required_states(model_name);
    }
    if (name == "derived.fast_neutron_fluence")
    {
        return fast_neutron_fluence_model_manager_.required_states(model_name);
    }
    throw std::out_of_range("尚未支持的派生量: " + name);
}

}
