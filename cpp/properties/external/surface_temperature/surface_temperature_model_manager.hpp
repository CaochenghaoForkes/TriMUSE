#pragma once

#include <string>
#include <unordered_map>

#include "properties/common/common_model_manager.hpp"

namespace trimuse
{

class SurfaceTemperatureModelManager
{
public:
    void register_models();

    void assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const;

    const std::vector<std::string>& required_states(const std::string& model_name) const;

private:
    CommonModelManager common_model_manager_;
    std::unordered_map<int, CommonModel> zone_model_map_;
};

}
