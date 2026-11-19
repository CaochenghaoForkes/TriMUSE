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
    interface_thermal_resistance_model_manager_.register_models();
}

void AssemblyModelManager::assign(const std::string& name, int attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    if (name == "assembly.interface_thermal_resistance")
    {
        interface_thermal_resistance_model_manager_.assign(attribute, model_name, parameters);
        return;
    }
    throw std::out_of_range("尚未支持的组装属性: " + name);
}

std::optional<StateValue> AssemblyModelManager::evaluate(const std::string& name, const PropertiesStatePoint& point) const
{
    if (name == "assembly.interface_thermal_resistance")
    {
        return interface_thermal_resistance_model_manager_.evaluate(point);
    }
    throw std::out_of_range("尚未支持的组装属性: " + name);
}

const std::vector<std::string>& AssemblyModelManager::required_states(const std::string& name, const std::string& model_name) const
{
    if (name == "assembly.interface_thermal_resistance")
    {
        return interface_thermal_resistance_model_manager_.required_states(model_name);
    }
    throw std::out_of_range("尚未支持的组装属性: " + name);
}

}
