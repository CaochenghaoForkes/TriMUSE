#include "properties/derived/heat_transfer_coef/heat_transfer_coef_model_manager.hpp"
#include "properties/derived/heat_transfer_coef/constant.hpp"
#include "properties/derived/heat_transfer_coef/time_series.hpp"

namespace trimuse
{

void HeatTransferCoefModelManager::register_models()
{
    registry_.register_model<ConstantHeatTransferCoef>("constant");
    registry_.register_model<TimeSeriesHeatTransferCoef>("time_series", {"time"});
}

void HeatTransferCoefModelManager::assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    zone_model_map_[zone_attribute] = registry_.registration(model_name).factory(parameters);
}

StateValue HeatTransferCoefModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return zone_model_map_.at(point.zone_attribute)->evaluate(point);
}

const std::vector<std::string>& HeatTransferCoefModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
