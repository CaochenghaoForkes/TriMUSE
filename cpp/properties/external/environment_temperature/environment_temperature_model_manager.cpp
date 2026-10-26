#include "properties/external/environment_temperature/environment_temperature_model_manager.hpp"
#include "properties/external/environment_temperature/constant_environment_temperature.hpp"
#include "properties/external/environment_temperature/time_series_environment_temperature.hpp"

namespace trimuse
{

void EnvironmentTemperatureModelManager::register_models()
{
    registry_.register_model<ConstantEnvironmentTemperature>("common.constant");
    registry_.register_model<TimeSeriesEnvironmentTemperature>("common.time_series", {"external.time"});
}

void EnvironmentTemperatureModelManager::assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    zone_model_map_[zone_attribute] = registry_.registration(model_name).factory(parameters, value_type);
}

StateValue EnvironmentTemperatureModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return zone_model_map_.at(point.zone_attribute)->evaluate(point);
}

const std::vector<std::string>& EnvironmentTemperatureModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
