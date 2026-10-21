#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "common/model_parameters.hpp"
#include "properties/common/constant.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

using CommonModel = std::variant<ConstantModel, TimeSeriesModel>;

class CommonModelManager
{
public:
    void register_models();

    CommonModel create(const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type) const;

    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    using Factory = std::function<CommonModel(const ModelParameterMap&, const std::string&)>;

    struct Registration
    {
        Factory factory;
        std::vector<std::string> required_states;
    };

    std::unordered_map<std::string, Registration> registrations_;
};

}
