#include "properties/external/common/common_external_model_manager.hpp"

namespace trimuse
{

namespace
{

VectorValue parameter_values(const ModelParameterValue& value)
{
    return std::holds_alternative<double>(value)
        ? VectorValue{std::get<double>(value)} : std::get<VectorValue>(value);
}

}

void CommonExternalModelManager::register_models()
{
    registrations_["constant"] = {[](const ModelParameterMap& parameters, const std::string& value_type) -> CommonExternalModel
    {
        return ConstantExternalState(parameters.at("constant_value"), value_type);
    }, {}};

    registrations_["time_series"] = {[](const ModelParameterMap& parameters, const std::string& value_type) -> CommonExternalModel
    {
        const VectorValue times = parameter_values(parameters.at("times"));
        const VectorValue samples = parameter_values(parameters.at("values"));
        const std::size_t components = value_type == "scalar" ? 1
            : value_type == "tensor2d" ? 4 : value_type == "tensor3d" ? 9 : samples.size() / times.size();

        std::vector<StateValue> values;
        values.reserve(times.size());
        for (std::size_t index = 0; index < times.size(); ++index)
        {
            const VectorValue sample(samples.begin() + index * components, samples.begin() + (index + 1) * components);
            values.push_back(ConstantExternalState(sample, value_type).evaluate(PropertiesStatePoint{}));
        }
        return TimeSeriesExternalState(times, values);
    }, {"time"}};
}

CommonExternalModel CommonExternalModelManager::create(const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type) const
{
    return registrations_.at(model_name).factory(parameters, value_type);
}

const std::vector<std::string>& CommonExternalModelManager::required_states(const std::string& model_name) const
{
    return registrations_.at(model_name).required_states;
}

}
