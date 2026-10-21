#include "properties/external/burnup_fima_fraction/burnup_fima_fraction_model_manager.hpp"

namespace trimuse
{

void BurnupFimaFractionModelManager::register_models()
{
    common_model_manager_.register_models();
}

void BurnupFimaFractionModelManager::assign(const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    model_ = common_model_manager_.create(model_name, parameters, value_type);
}

StateValue BurnupFimaFractionModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return std::visit([&point](const auto& model)
    {
        return model.evaluate(point);
    }, model_.value());
}

const std::vector<std::string>& BurnupFimaFractionModelManager::required_states(const std::string& model_name) const
{
    return common_model_manager_.required_states(model_name);
}

}
