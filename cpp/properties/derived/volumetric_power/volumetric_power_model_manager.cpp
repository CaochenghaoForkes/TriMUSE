#include "properties/derived/volumetric_power/volumetric_power_model_manager.hpp"
#include "properties/derived/volumetric_power/uniform.hpp"

namespace trimuse
{

void VolumetricPowerModelManager::register_models()
{
    registry_.register_model<UniformVolumetricPower>("uniform", {"power", "region_volume"});
}

void VolumetricPowerModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

StateValue VolumetricPowerModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<std::string>& VolumetricPowerModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
