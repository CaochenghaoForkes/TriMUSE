#include "properties/material/mechanics/shear_modulus/shear_modulus_model_manager.hpp"
#include "properties/material/mechanics/shear_modulus/common/isotropy_shear_modulus.hpp"
#include "properties/material/mechanics/shear_modulus/common/constant_shear_modulus.hpp"
#include "properties/material/mechanics/shear_modulus/common/time_series_shear_modulus.hpp"

namespace trimuse
{

void ShearModulusModelManager::register_models()
{
    registry_.register_model<ConstantShearModulus>("common.constant");
    registry_.register_model<TimeSeriesShearModulus>("common.time_series", {"external.time"});
    registry_.register_model<IsotropyShearModulus>("common.isotropy", {"material.mechanics.young_modulus", "material.mechanics.poisson_ratio"});
}

void ShearModulusModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double ShearModulusModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<std::string>& ShearModulusModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
