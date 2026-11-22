#pragma once

#include <string>
#include <vector>

#include "properties/material/general/mass_density/mass_density_model_manager.hpp"
#include "properties/material/general/crystallite_diameter/crystallite_diameter_model_manager.hpp"
#include "properties/material/general/porosity/porosity_model_manager.hpp"
#include "properties/material/general/initial_bacon_anisotropy_factor/initial_bacon_anisotropy_factor_model_manager.hpp"
#include "properties/material/general/bacon_anisotropy_factor/bacon_anisotropy_factor_model_manager.hpp"

namespace trimuse
{

class GeneralModelManager
{
public:
    void register_models();
    void assign(int region_attribute, const std::string& property_name, const std::string& model_name, const ModelParameterMap& parameters);
    StateValue evaluate(const std::string& property_name, const PropertiesStatePoint& state_point) const;
    const std::vector<RequiredState>& required_states(const std::string& property_name, const std::string& model_name) const;

private:
    PorosityModelManager porosity_model_manager_;
    MassDensityModelManager mass_density_model_manager_;
    CrystalliteDiameterModelManager crystallite_diameter_model_manager_;
    InitialBaconAnisotropyFactorModelManager initial_bacon_anisotropy_factor_model_manager_;
    BaconAnisotropyFactorModelManager bacon_anisotropy_factor_model_manager_;
};

}
