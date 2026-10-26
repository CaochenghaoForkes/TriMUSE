#include "properties/derived/volumetric_nuclide_birth_rate/volumetric_nuclide_birth_rate_model_manager.hpp"
#include "properties/derived/volumetric_nuclide_birth_rate/uniform_from_birth_rate.hpp"
#include "properties/derived/volumetric_nuclide_birth_rate/constant_volumetric_nuclide_birth_rate.hpp"
#include "properties/derived/volumetric_nuclide_birth_rate/time_series_volumetric_nuclide_birth_rate.hpp"

namespace trimuse
{

void VolumetricNuclideBirthRateModelManager::register_models()
{
    registry_.register_model<ConstantVolumetricNuclideBirthRate>("constant");
    registry_.register_model<TimeSeriesVolumetricNuclideBirthRate>("time_series", {"time"});
    registry_.register_model<UniformFromBirthRate>("uniform_from_birth_rate", {"nuclide_birth_rate", "uranium_fraction", "region_volume"});
}

void VolumetricNuclideBirthRateModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

StateValue VolumetricNuclideBirthRateModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<std::string>& VolumetricNuclideBirthRateModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
