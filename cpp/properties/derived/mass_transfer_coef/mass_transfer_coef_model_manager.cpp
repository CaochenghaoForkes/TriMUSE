#include "properties/derived/mass_transfer_coef/mass_transfer_coef_model_manager.hpp"
#include "properties/common/common_model_adapter.hpp"

namespace trimuse
{

void MassTransferCoefModelManager::register_models()
{
    register_common_models<MassTransferCoefModel, StateValue>(registry_, "constant", "time_series");
}

void MassTransferCoefModelManager::assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    zone_model_map_[zone_attribute] = registry_.registration(model_name).factory(parameters);
}

StateValue MassTransferCoefModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return zone_model_map_.at(point.zone_attribute)->evaluate(point);
}

const std::vector<std::string>& MassTransferCoefModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
