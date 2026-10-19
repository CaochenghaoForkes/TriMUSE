#pragma once

#include <string>
#include <vector>

#include "properties/material/general/mass_density/mass_density_model_manager.hpp"
#include "properties/material/general/porosity/porosity_model_manager.hpp"

namespace trimuse
{

class GeneralModelManager
{
public:
    void register_models();

    void assign(int region_attribute, const std::string& property_name, const std::string& model_name, const ModelParameterMap& parameters);

    StateValue evaluate(const std::string& property_name, const PropertiesStatePoint& state_point) const;

    const std::vector<std::string>& required_states(const std::string& property_name, const std::string& model_name) const;

private:
    PorosityModelManager porosity_model_manager_;
    MassDensityModelManager mass_density_model_manager_;
};

}
