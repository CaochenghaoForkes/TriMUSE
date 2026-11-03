#pragma once

#include <string>
#include <vector>

#include "properties/derived/heat_transfer_coef/heat_transfer_coef_model_manager.hpp"
#include "properties/derived/mass_transfer_coef/mass_transfer_coef_model_manager.hpp"
#include "properties/derived/volumetric_power/volumetric_power_model_manager.hpp"
#include "properties/derived/volumetric_nuclide_birth_rate/volumetric_nuclide_birth_rate_model_manager.hpp"
#include "properties/derived/fast_neutron_fluence/fast_neutron_fluence_model_manager.hpp"

namespace trimuse
{

class DerivedModelManager
{
public:
    DerivedModelManager();

    void register_models();

    void assign(const std::string& name, int attribute, const std::string& model_name, const ModelParameterMap& parameters);

    StateValue evaluate(const std::string& name, const PropertiesStatePoint& point) const;

    const std::vector<std::string>& required_states(const std::string& name, const std::string& model_name) const;

private:
    VolumetricPowerModelManager volumetric_power_model_manager_;
    HeatTransferCoefModelManager heat_transfer_coef_model_manager_;
    MassTransferCoefModelManager mass_transfer_coef_model_manager_;
    VolumetricNuclideBirthRateModelManager volumetric_nuclide_birth_rate_model_manager_;
    FastNeutronFluenceModelManager fast_neutron_fluence_model_manager_;

};

}
