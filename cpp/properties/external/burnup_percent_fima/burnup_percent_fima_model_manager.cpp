#include "properties/external/burnup_percent_fima/burnup_percent_fima_model_manager.hpp"

namespace trimuse
{

void BurnupPercentFimaModelManager::register_models()
{
    common_model_manager_.register_models();
}

void BurnupPercentFimaModelManager::assign(const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    model_ = common_model_manager_.create(model_name, parameters, value_type);
}

StateValue BurnupPercentFimaModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return std::visit([&point](const auto& model)
    {
        return model.evaluate(point);
    }, model_.value());
}

const std::vector<std::string>& BurnupPercentFimaModelManager::required_states(const std::string& model_name) const
{
    return common_model_manager_.required_states(model_name);
}

}
