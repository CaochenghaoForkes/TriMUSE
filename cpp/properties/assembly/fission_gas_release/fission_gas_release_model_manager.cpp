#include "properties/assembly/fission_gas_release/fission_gas_release_model_manager.hpp"
#include "properties/assembly/fission_gas_release/booth.hpp"

namespace trimuse
{

FissionGasReleaseModel::~FissionGasReleaseModel() = default;

void FissionGasReleaseModelManager::register_models()
{
    registry_.register_model<BoothFissionGasRelease>("common.booth", {"assembly.fission_gas_release", "material.radioactivity.fission_gas_diffusion_coef", "derived.fission_gas_production_rate", "derived.region_volume", "external.time_step"});
}

void FissionGasReleaseModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

VectorValue FissionGasReleaseModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<RequiredState>& FissionGasReleaseModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
