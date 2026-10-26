#include "properties/external/power/power_model_manager.hpp"
#include "properties/external/power/constant_power.hpp"
#include "properties/external/power/time_series_power.hpp"

namespace trimuse
{

void PowerModelManager::register_models()
{
    registry_.register_model<ConstantPower>("constant");
    registry_.register_model<TimeSeriesPower>("time_series", {"time"});
}

void PowerModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters, value_type);
}

StateValue PowerModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<std::string>& PowerModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
