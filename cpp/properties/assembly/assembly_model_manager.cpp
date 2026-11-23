#include "properties/assembly/assembly_model_manager.hpp"

#include <stdexcept>

namespace trimuse
{

AssemblyModelManager::AssemblyModelManager()
{
    register_models();
}

void AssemblyModelManager::register_models()
{
    internal_gas_pressure_model_manager_.register_models();
    cumulative_co_release_model_manager_.register_models();
    local_panama_opf_model_manager_.register_models();
    homogenized_panama_opf_model_manager_.register_models();
    local_booth_fission_gas_release_state_model_manager_.register_models();
    homogenized_booth_fission_gas_release_state_model_manager_.register_models();
    cumulative_fission_gas_release_model_manager_.register_models();
    interface_thermal_resistance_model_manager_.register_models();
}

void AssemblyModelManager::assign(const std::string& name, int attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    if (name == "assembly.internal_gas_pressure")
    {
        internal_gas_pressure_model_manager_.assign(attribute, model_name, parameters);
        return;
    }
    if (name == "assembly.cumulative_co_release")
    {
        cumulative_co_release_model_manager_.assign(attribute, model_name, parameters);
        return;
    }
    if (name == "assembly.homogenized_panama_opf")
    {
        homogenized_panama_opf_model_manager_.assign(attribute, model_name, parameters);
        return;
    }
    if (name == "assembly.local_panama_opf")
    {
        local_panama_opf_model_manager_.assign(attribute, model_name, parameters);
        return;
    }
    if (name == "assembly.homogenized_booth_fission_gas_release_state")
    {
        homogenized_booth_fission_gas_release_state_model_manager_.assign(attribute, model_name, parameters);
        return;
    }
    if (name == "assembly.cumulative_fission_gas_release")
    {
        cumulative_fission_gas_release_model_manager_.assign(attribute, model_name, parameters);
        return;
    }
    if (name == "assembly.local_booth_fission_gas_release_state")
    {
        local_booth_fission_gas_release_state_model_manager_.assign(attribute, model_name, parameters);
        return;
    }
    if (name == "assembly.interface_thermal_resistance")
    {
        interface_thermal_resistance_model_manager_.assign(attribute, model_name, parameters);
        return;
    }
    throw std::out_of_range("尚未支持的组装属性: " + name);
}

std::optional<StateValue> AssemblyModelManager::evaluate(const std::string& name, const PropertiesStatePoint& point) const
{
    if (name == "assembly.internal_gas_pressure")
    {
        return internal_gas_pressure_model_manager_.evaluate(point);
    }
    if (name == "assembly.cumulative_co_release")
    {
        return cumulative_co_release_model_manager_.evaluate(point);
    }
    if (name == "assembly.homogenized_panama_opf")
    {
        return homogenized_panama_opf_model_manager_.evaluate(point);
    }
    if (name == "assembly.local_panama_opf")
    {
        return local_panama_opf_model_manager_.evaluate(point);
    }
    if (name == "assembly.homogenized_booth_fission_gas_release_state")
    {
        return homogenized_booth_fission_gas_release_state_model_manager_.evaluate(point);
    }
    if (name == "assembly.cumulative_fission_gas_release")
    {
        return cumulative_fission_gas_release_model_manager_.evaluate(point);
    }
    if (name == "assembly.local_booth_fission_gas_release_state")
    {
        return local_booth_fission_gas_release_state_model_manager_.evaluate(point);
    }
    if (name == "assembly.interface_thermal_resistance")
    {
        return interface_thermal_resistance_model_manager_.evaluate(point);
    }
    throw std::out_of_range("尚未支持的组装属性: " + name);
}

const std::vector<RequiredState>& AssemblyModelManager::required_states(const std::string& name, const std::string& model_name) const
{
    if (name == "assembly.internal_gas_pressure")
    {
        return internal_gas_pressure_model_manager_.required_states(model_name);
    }
    if (name == "assembly.cumulative_co_release")
    {
        return cumulative_co_release_model_manager_.required_states(model_name);
    }
    if (name == "assembly.homogenized_panama_opf")
    {
        return homogenized_panama_opf_model_manager_.required_states(model_name);
    }
    if (name == "assembly.local_panama_opf")
    {
        return local_panama_opf_model_manager_.required_states(model_name);
    }
    if (name == "assembly.homogenized_booth_fission_gas_release_state")
    {
        return homogenized_booth_fission_gas_release_state_model_manager_.required_states(model_name);
    }
    if (name == "assembly.cumulative_fission_gas_release")
    {
        return cumulative_fission_gas_release_model_manager_.required_states(model_name);
    }
    if (name == "assembly.local_booth_fission_gas_release_state")
    {
        return local_booth_fission_gas_release_state_model_manager_.required_states(model_name);
    }
    if (name == "assembly.interface_thermal_resistance")
    {
        return interface_thermal_resistance_model_manager_.required_states(model_name);
    }
    throw std::out_of_range("尚未支持的组装属性: " + name);
}

}
