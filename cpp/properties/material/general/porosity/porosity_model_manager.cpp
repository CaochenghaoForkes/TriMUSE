#include "properties/material/general/porosity/porosity_model_manager.hpp"
#include "properties/common/common_model_adapter.hpp"

namespace trimuse
{

void PorosityModelManager::register_models()
{
    register_common_models<PorosityModel>(registry_);
}

void PorosityModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double PorosityModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<std::string>& PorosityModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
