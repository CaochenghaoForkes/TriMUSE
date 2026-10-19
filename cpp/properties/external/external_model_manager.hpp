#pragma once

#include <string>
#include <vector>

#include "properties/external/burnup_percent_fima/burnup_percent_fima_model_manager.hpp"
#include "properties/external/environment_temperature/environment_temperature_model_manager.hpp"
#include "properties/external/heat_flux/heat_flux_model_manager.hpp"
#include "properties/external/power/power_model_manager.hpp"

namespace trimuse
{

class ExternalModelManager
{
public:
    ExternalModelManager();

    void register_models();

    void assign(const std::string& name, int attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type = "scalar");

    StateValue evaluate(const std::string& name, const PropertiesStatePoint& point) const;

    const std::vector<std::string>& required_states(const std::string& name, const std::string& model_name) const;

private:
    PowerModelManager power_model_manager_;
    EnvironmentTemperatureModelManager environment_temperature_model_manager_;
    HeatFluxModelManager heat_flux_model_manager_;
    BurnupPercentFimaModelManager burnup_percent_fima_model_manager_;
};

}
