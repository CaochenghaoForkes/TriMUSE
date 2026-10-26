#include "properties/external/heat_flux/heat_flux_model_manager.hpp"
#include "properties/external/heat_flux/constant_heat_flux.hpp"
#include "properties/external/heat_flux/time_series_heat_flux.hpp"

namespace trimuse
{

void HeatFluxModelManager::register_models()
{
    registry_.register_model<ConstantHeatFlux>("constant");
    registry_.register_model<TimeSeriesHeatFlux>("time_series", {"time"});
}

void HeatFluxModelManager::assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    zone_model_map_[zone_attribute] = registry_.registration(model_name).factory(parameters, value_type);
}

StateValue HeatFluxModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return zone_model_map_.at(point.zone_attribute)->evaluate(point);
}

const std::vector<std::string>& HeatFluxModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
