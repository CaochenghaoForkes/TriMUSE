#pragma once

#include <string>
#include <vector>

#include "properties/material/general/general_model_manager.hpp"
#include "properties/material/thermal/thermal_model_manager.hpp"

namespace trimuse
{

class MaterialModelManager
{
public:
    MaterialModelManager();

    void register_models();

    void assign(int region_attribute, const std::string& property_name, const std::string& model_name, const ModelParameterMap& parameters);

    StateValue evaluate(const std::string& property_name, const PropertiesStatePoint& state_point) const;

    const std::vector<std::string>& required_states(const std::string& property_name, const std::string& model_name) const;

private:
    GeneralModelManager general_model_manager_;
    ThermalModelManager thermal_model_manager_;
};

}
