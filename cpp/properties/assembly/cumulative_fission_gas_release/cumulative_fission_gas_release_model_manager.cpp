#include "properties/assembly/cumulative_fission_gas_release/cumulative_fission_gas_release_model_manager.hpp"
#include "properties/assembly/cumulative_fission_gas_release/from_local_fission_gas_release.hpp"
#include "properties/assembly/cumulative_fission_gas_release/from_homogenized_fission_gas_release.hpp"

namespace trimuse
{

CumulativeFissionGasReleaseModel::~CumulativeFissionGasReleaseModel() = default;

void CumulativeFissionGasReleaseModelManager::register_models()
{
    registry_.register_model<FromHomogenizedFissionGasRelease>("common.from_homogenized_fission_gas_release", {"assembly.homogenized_fission_gas_release_state", "derived.region_volume"});
    registry_.register_model<FromLocalFissionGasRelease>("common.from_local_fission_gas_release", {{"assembly.local_fission_gas_release_state", true}, {"precomputed.initial_domain_quadrature_weight", true}});
}

void CumulativeFissionGasReleaseModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double CumulativeFissionGasReleaseModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<RequiredState>& CumulativeFissionGasReleaseModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
