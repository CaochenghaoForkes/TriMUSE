#include "properties/external/uranium_fraction/uranium_fraction_model_manager.hpp"
#include "properties/external/uranium_fraction/constant_uranium_fraction.hpp"
#include "properties/external/uranium_fraction/time_series_uranium_fraction.hpp"

namespace trimuse
{

void UraniumFractionModelManager::register_models()
{
    registry_.register_model<ConstantUraniumFraction>("constant");
    registry_.register_model<TimeSeriesUraniumFraction>("time_series", {"time"});
}

void UraniumFractionModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters, value_type);
}

StateValue UraniumFractionModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<std::string>& UraniumFractionModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
