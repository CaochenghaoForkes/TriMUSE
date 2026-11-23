#pragma once

#include "properties/properties_state_point.hpp"
#include "properties/assembly/cumulative_fission_gas_release/cumulative_fission_gas_release_model_manager.hpp"
#include "properties/assembly/fission_gas_release/fission_gas_release_model_manager.hpp"
#include "properties/assembly/interface_pressure/interface_pressure_model_manager.hpp"
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
    const std::vector<RequiredState>& required_states(const std::string& name, const std::string& model_name) const;

private:
    FissionGasReleaseModelManager fission_gas_release_model_manager_;
    CumulativeFissionGasReleaseModelManager cumulative_fission_gas_release_model_manager_;
    InterfaceThermalResistanceModelManager interface_thermal_resistance_model_manager_;
    InterfacePressureModelManager interface_pressure_model_manager_;
};

}
