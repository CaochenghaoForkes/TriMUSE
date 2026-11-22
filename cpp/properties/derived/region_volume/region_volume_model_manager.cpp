#include "properties/derived/region_volume/region_volume_model_manager.hpp"
#include "properties/derived/region_volume/undeformed.hpp"

namespace trimuse
{

RegionVolumeModel::~RegionVolumeModel() = default;

void RegionVolumeModelManager::register_models()
{
    registry_.register_model<Undeformed>("common.undeformed", {"precomputed.initial_region_volume"});
}

void RegionVolumeModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

StateValue RegionVolumeModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<RequiredState>& RegionVolumeModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
