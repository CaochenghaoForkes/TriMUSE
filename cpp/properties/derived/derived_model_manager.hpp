#pragma once

#include <string>
#include <vector>

#include "properties/derived/irradiation_time/irradiation_time_model_manager.hpp"

#include "properties/derived/fission_gas_production_rate/fission_gas_production_rate_model_manager.hpp"
#include "properties/derived/average_volumetric_fission_rate/average_volumetric_fission_rate_model_manager.hpp"
#include "properties/derived/region_volume/region_volume_model_manager.hpp"
#include "properties/derived/burnup_fima_fraction/burnup_fima_fraction_model_manager.hpp"
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
    const std::vector<RequiredState>& required_states(const std::string& name, const std::string& model_name) const;

private:
    IrradiationTimeModelManager irradiation_time_model_manager_;
    RegionVolumeModelManager region_volume_model_manager_;
    BurnupFimaFractionModelManager burnup_fima_fraction_model_manager_;
    VolumetricPowerModelManager volumetric_power_model_manager_;
    AverageVolumetricFissionRateModelManager average_volumetric_fission_rate_model_manager_;
    FissionGasProductionRateModelManager fission_gas_production_rate_model_manager_;
    HeatTransferCoefModelManager heat_transfer_coef_model_manager_;
    MassTransferCoefModelManager mass_transfer_coef_model_manager_;
    VolumetricNuclideBirthRateModelManager volumetric_nuclide_birth_rate_model_manager_;
    FastNeutronFluenceModelManager fast_neutron_fluence_model_manager_;
};

}
