#pragma once

#include <optional>
#include <string>

#include "properties/common/common_model_manager.hpp"

namespace trimuse
{

class BurnupFimaFractionModelManager
{
public:
    void register_models();

    void assign(const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const;

    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    CommonModelManager common_model_manager_;
    std::optional<CommonModel> model_;
};

}
