#pragma once

#include "properties/properties_state_point.hpp"
#include "properties/assembly/internal_gas_pressure/internal_gas_pressure_model_manager.hpp"
#include "properties/assembly/cumulative_co_release/cumulative_co_release_model_manager.hpp"
#include "properties/assembly/homogenized_panama_opf/homogenized_panama_opf_model_manager.hpp"
#include "properties/assembly/local_panama_opf/local_panama_opf_model_manager.hpp"
#include "properties/assembly/homogenized_booth_fission_gas_release_state/homogenized_booth_fission_gas_release_state_model_manager.hpp"
#include "properties/assembly/cumulative_fission_gas_release/cumulative_fission_gas_release_model_manager.hpp"
#include "properties/assembly/local_booth_fission_gas_release_state/local_booth_fission_gas_release_state_model_manager.hpp"
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
    InternalGasPressureModelManager internal_gas_pressure_model_manager_;
    CumulativeCoReleaseModelManager cumulative_co_release_model_manager_;
    LocalPanamaOpfModelManager local_panama_opf_model_manager_;
    HomogenizedPanamaOpfModelManager homogenized_panama_opf_model_manager_;
    LocalBoothFissionGasReleaseStateModelManager local_booth_fission_gas_release_state_model_manager_;
    HomogenizedBoothFissionGasReleaseStateModelManager homogenized_booth_fission_gas_release_state_model_manager_;
    CumulativeFissionGasReleaseModelManager cumulative_fission_gas_release_model_manager_;
    InterfaceThermalResistanceModelManager interface_thermal_resistance_model_manager_;
};

}
