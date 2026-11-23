#include "properties/assembly/local_fission_gas_release_state/local_fission_gas_release_state_model_manager.hpp"
#include "properties/assembly/local_fission_gas_release_state/booth.hpp"

namespace trimuse
{

LocalFissionGasReleaseStateModel::~LocalFissionGasReleaseStateModel() = default;

void LocalFissionGasReleaseStateModelManager::register_models()
{
    registry_.register_model<BoothLocalFissionGasReleaseState>("common.booth", {"assembly.local_fission_gas_release_state", "material.radioactivity.fission_gas_diffusion_coef", "derived.fission_gas_production_rate", "derived.region_volume", "external.time_step"});
}

void LocalFissionGasReleaseStateModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

VectorValue LocalFissionGasReleaseStateModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<RequiredState>& LocalFissionGasReleaseStateModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
