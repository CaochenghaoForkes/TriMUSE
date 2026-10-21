#pragma once

#include <string>
#include <vector>

#include "properties/material/radioactivity/nuclide_diffusion_coef/nuclide_diffusion_coef_model_manager.hpp"

namespace trimuse
{

class RadioactivityModelManager
{
public:
    void register_models();

    void assign(int region_attribute, const std::string& property_name, const std::string& model_name, const ModelParameterMap& parameters);

    StateValue evaluate(const std::string& property_name, const PropertiesStatePoint& state_point) const;

    const std::vector<std::string>& required_states(const std::string& property_name, const std::string& model_name) const;

private:
    NuclideDiffusionCoefModelManager nuclide_diffusion_coef_model_manager_;
};

}
