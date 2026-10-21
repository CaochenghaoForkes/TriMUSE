#include "properties/common/common_model_manager.hpp"

namespace trimuse
{

void CommonModelManager::register_models()
{
    registrations_["constant"] = {[](const ModelParameterMap& parameters, const std::string& value_type) -> CommonModel
    {
        return ConstantModel(parameters, value_type);
    }, {}};

    registrations_["time_series"] = {[](const ModelParameterMap& parameters, const std::string& value_type) -> CommonModel
    {
        return TimeSeriesModel(parameters, value_type);
    }, {"time"}};
}

CommonModel CommonModelManager::create(const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type) const
{
    return registrations_.at(model_name).factory(parameters, value_type);
}

const std::vector<std::string>& CommonModelManager::required_states(const std::string& model_name) const
{
    return registrations_.at(model_name).required_states;
}

}
