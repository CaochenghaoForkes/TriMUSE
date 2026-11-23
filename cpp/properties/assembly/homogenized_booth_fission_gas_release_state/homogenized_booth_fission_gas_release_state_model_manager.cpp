#include "properties/assembly/homogenized_booth_fission_gas_release_state/homogenized_booth_fission_gas_release_state_model_manager.hpp"
#include "properties/assembly/homogenized_booth_fission_gas_release_state/booth.hpp"

namespace trimuse
{

HomogenizedBoothFissionGasReleaseStateModel::~HomogenizedBoothFissionGasReleaseStateModel() = default;

void HomogenizedBoothFissionGasReleaseStateModelManager::register_models()
{
    registry_.register_model<BoothHomogenizedFissionGasReleaseState>("common.booth", {"assembly.homogenized_booth_fission_gas_release_state", {"material.radioactivity.fission_gas_diffusion_coef", true}, {"precomputed.initial_domain_quadrature_weight", true}, "derived.fission_gas_production_rate", "derived.region_volume", "external.time_step"});
}

void HomogenizedBoothFissionGasReleaseStateModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

VectorValue HomogenizedBoothFissionGasReleaseStateModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<RequiredState>& HomogenizedBoothFissionGasReleaseStateModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
