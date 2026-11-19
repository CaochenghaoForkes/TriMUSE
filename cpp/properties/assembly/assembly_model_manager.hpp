#pragma once

#include "properties/properties_state_point.hpp"
#include "properties/assembly/interface_thermal_resistance/interface_thermal_resistance_model_manager.hpp"

namespace trimuse
{

class AssemblyModelManager
{
public:
    AssemblyModelManager();

    void register_models();

    void assign(const std::string& name, int attribute, const std::string& model_name, const ModelParameterMap& parameters);

    std::optional<StateValue> evaluate(const std::string& name, const PropertiesStatePoint& point) const;

    const std::vector<std::string>& required_states(const std::string& name, const std::string& model_name) const;

private:
    InterfaceThermalResistanceModelManager interface_thermal_resistance_model_manager_;
};

}
