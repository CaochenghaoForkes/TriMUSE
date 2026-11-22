#include "properties/material/radioactivity/fission_gas_diffusion_coef/fission_gas_diffusion_coef_model_manager.hpp"
#include "properties/material/radioactivity/fission_gas_diffusion_coef/common/arrhenius_fission_gas_diffusion_coef.hpp"
#include "properties/material/radioactivity/fission_gas_diffusion_coef/common/constant_fission_gas_diffusion_coef.hpp"
#include "properties/material/radioactivity/fission_gas_diffusion_coef/common/time_series_fission_gas_diffusion_coef.hpp"
#include "properties/material/radioactivity/fission_gas_diffusion_coef/kernel/bison_2021_fission_gas_diffusion_coef.hpp"

namespace trimuse
{

FissionGasDiffusionCoefModel::~FissionGasDiffusionCoefModel() = default;

void FissionGasDiffusionCoefModelManager::register_models()
{
    registry_.register_model<ConstantFissionGasDiffusionCoef>("common.constant");
    registry_.register_model<TimeSeriesFissionGasDiffusionCoef>("common.time_series", {"external.time"});
    registry_.register_model<ArrheniusFissionGasDiffusionCoef>("common.arrhenius", {"temperature"});
    registry_.register_model<Bison2021FissionGasDiffusionCoef>("kernel.bison_2021", {"temperature", "derived.burnup_fima_fraction", "derived.average_volumetric_fission_rate", "derived.irradiation_time"});
}

void FissionGasDiffusionCoefModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double FissionGasDiffusionCoefModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<RequiredState>& FissionGasDiffusionCoefModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
