#include "properties/derived/average_volumetric_fission_rate/average_volumetric_fission_rate_model_manager.hpp"
#include "properties/derived/average_volumetric_fission_rate/constant_average_volumetric_fission_rate.hpp"
#include "properties/derived/average_volumetric_fission_rate/time_series_average_volumetric_fission_rate.hpp"
#include "properties/derived/average_volumetric_fission_rate/from_power.hpp"

namespace trimuse
{

AverageVolumetricFissionRateModel::~AverageVolumetricFissionRateModel() = default;

void AverageVolumetricFissionRateModelManager::register_models()
{
    registry_.register_model<ConstantAverageVolumetricFissionRate>("common.constant");
    registry_.register_model<TimeSeriesAverageVolumetricFissionRate>("common.time_series", {"external.time"});
    registry_.register_model<FromPower>("common.from_power", {"external.power", "derived.region_volume", "external.energy_per_fission"});
}

void AverageVolumetricFissionRateModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

StateValue AverageVolumetricFissionRateModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<RequiredState>& AverageVolumetricFissionRateModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
