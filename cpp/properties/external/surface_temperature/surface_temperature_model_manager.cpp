#include "properties/external/surface_temperature/surface_temperature_model_manager.hpp"

namespace trimuse
{

void SurfaceTemperatureModelManager::register_models()
{
    common_model_manager_.register_models();
}

void SurfaceTemperatureModelManager::assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    zone_model_map_.insert_or_assign(zone_attribute, common_model_manager_.create(model_name, parameters, value_type));
}

StateValue SurfaceTemperatureModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return std::visit([&point](const auto& model)
    {
        return model.evaluate(point);
    }, zone_model_map_.at(point.zone_attribute));
}

const std::vector<std::string>& SurfaceTemperatureModelManager::required_states(const std::string& model_name) const
{
    return common_model_manager_.required_states(model_name);
}

}
