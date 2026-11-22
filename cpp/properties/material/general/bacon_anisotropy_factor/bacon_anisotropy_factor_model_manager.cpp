#include "properties/material/general/bacon_anisotropy_factor/bacon_anisotropy_factor_model_manager.hpp"
#include "properties/material/general/bacon_anisotropy_factor/common/constant_bacon_anisotropy_factor.hpp"
#include "properties/material/general/bacon_anisotropy_factor/common/time_series_bacon_anisotropy_factor.hpp"
#include "properties/material/general/bacon_anisotropy_factor/pyc/bison_2021_bacon_anisotropy_factor.hpp"

namespace trimuse
{

BaconAnisotropyFactorModel::~BaconAnisotropyFactorModel() = default;

void BaconAnisotropyFactorModelManager::register_models()
{
    registry_.register_model<ConstantBaconAnisotropyFactor>("common.constant");
    registry_.register_model<TimeSeriesBaconAnisotropyFactor>("common.time_series", {"external.time"});
    registry_.register_model<Bison2021BaconAnisotropyFactor>("pyc.bison_2021", {"material.general.initial_bacon_anisotropy_factor", "derived.fast_neutron_fluence"});
}

void BaconAnisotropyFactorModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double BaconAnisotropyFactorModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<RequiredState>& BaconAnisotropyFactorModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
