#include "properties/material/general/mass_density/mass_density_model_manager.hpp"
#include "properties/common/common_model_adapter.hpp"

namespace trimuse
{

void MassDensityModelManager::register_models()
{
    register_common_models<MassDensityModel>(registry_);
}

void MassDensityModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double MassDensityModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<std::string>& MassDensityModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
