#include "properties/external/surface_temperature/surface_temperature_model_manager.hpp"
#include "properties/external/surface_temperature/constant_surface_temperature.hpp"
#include "properties/external/surface_temperature/time_series_surface_temperature.hpp"

namespace trimuse
{

void SurfaceTemperatureModelManager::register_models()
{
    registry_.register_model<ConstantSurfaceTemperature>("constant");
    registry_.register_model<TimeSeriesSurfaceTemperature>("time_series", {"time"});
}

void SurfaceTemperatureModelManager::assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    zone_model_map_[zone_attribute] = registry_.registration(model_name).factory(parameters, value_type);
}

StateValue SurfaceTemperatureModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return zone_model_map_.at(point.zone_attribute)->evaluate(point);
}

const std::vector<std::string>& SurfaceTemperatureModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
