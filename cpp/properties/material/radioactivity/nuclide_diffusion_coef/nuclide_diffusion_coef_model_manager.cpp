#include "properties/material/radioactivity/nuclide_diffusion_coef/nuclide_diffusion_coef_model_manager.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/common/arrhenius_nuclide_diffusion_coef.hpp"
#include "properties/common/common_model_adapter.hpp"

namespace trimuse
{

void NuclideDiffusionCoefModelManager::register_models()
{
    register_common_models<NuclideDiffusionCoefModel>(registry_);
    registry_.register_model<ArrheniusNuclideDiffusionCoef>("common.arrhenius", {"temperature"});
}

void NuclideDiffusionCoefModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double NuclideDiffusionCoefModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<std::string>& NuclideDiffusionCoefModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
