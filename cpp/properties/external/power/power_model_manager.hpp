#pragma once

#include <string>
#include <unordered_map>

#include "properties/external/common/common_external_model_manager.hpp"

namespace trimuse
{

class PowerModelManager
{
public:
    void register_models();

    void assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const;

    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    CommonExternalModelManager common_model_manager_;

    std::unordered_map<int, CommonExternalModel> region_model_map_;
};

}
