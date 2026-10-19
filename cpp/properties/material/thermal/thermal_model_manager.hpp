#pragma once

#include <string>
#include <vector>

#include "properties/material/thermal/conductivity/conductivity_model_manager.hpp"
#include "properties/material/thermal/specific_heat/specific_heat_model_manager.hpp"

namespace trimuse
{

class ThermalModelManager
{
public:
    void register_models();

    void assign(int region_attribute, const std::string& property_name, const std::string& model_name, const ModelParameterMap& parameters);

    StateValue evaluate(const std::string& property_name, const PropertiesStatePoint& state_point) const;

    const std::vector<std::string>& required_states(const std::string& property_name, const std::string& model_name) const;

private:
    ConductivityModelManager conductivity_model_manager_;
    SpecificHeatModelManager specific_heat_model_manager_;
};

}
