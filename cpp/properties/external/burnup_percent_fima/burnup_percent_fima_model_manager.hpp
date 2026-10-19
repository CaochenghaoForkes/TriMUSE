#pragma once

#include <optional>
#include <string>

#include "properties/external/common/common_external_model_manager.hpp"

namespace trimuse
{

class BurnupPercentFimaModelManager
{
public:
    void register_models();

    void assign(const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const;

    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    CommonExternalModelManager common_model_manager_;
    std::optional<CommonExternalModel> model_;
};

}
