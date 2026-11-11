#include "properties/material/general/initial_bacon_anisotropy_factor/initial_bacon_anisotropy_factor_model_manager.hpp"
#include "properties/material/general/initial_bacon_anisotropy_factor/common/constant_initial_bacon_anisotropy_factor.hpp"

namespace trimuse
{

InitialBaconAnisotropyFactorModel::~InitialBaconAnisotropyFactorModel() = default;

void InitialBaconAnisotropyFactorModelManager::register_models()
{
    registry_.register_model<ConstantInitialBaconAnisotropyFactor>("common.constant");
}

void InitialBaconAnisotropyFactorModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double InitialBaconAnisotropyFactorModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<std::string>& InitialBaconAnisotropyFactorModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
