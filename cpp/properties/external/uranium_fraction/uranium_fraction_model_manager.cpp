#include "properties/external/uranium_fraction/uranium_fraction_model_manager.hpp"

namespace trimuse
{

void UraniumFractionModelManager::register_models()
{
    common_model_manager_.register_models();
}

void UraniumFractionModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    region_model_map_.insert_or_assign(region_attribute, common_model_manager_.create(model_name, parameters, value_type));
}

StateValue UraniumFractionModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return std::visit([&point](const auto& model)
    {
        return model.evaluate(point);
    }, region_model_map_.at(point.region_attribute));
}

const std::vector<std::string>& UraniumFractionModelManager::required_states(const std::string& model_name) const
{
    return common_model_manager_.required_states(model_name);
}

}
