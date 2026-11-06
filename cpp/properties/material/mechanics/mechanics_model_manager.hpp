#pragma once

#include <string>
#include <vector>

#include "properties/material/mechanics/young_modulus/young_modulus_model_manager.hpp"
#include "properties/material/mechanics/poisson_ratio/poisson_ratio_model_manager.hpp"
#include "properties/material/mechanics/shear_modulus/shear_modulus_model_manager.hpp"
#include "properties/material/mechanics/elasticity_tensor/elasticity_tensor_model_manager.hpp"

namespace trimuse
{

class MechanicsModelManager
{
public:
    void register_models();

    void assign(int region_attribute, const std::string& property_name, const std::string& model_name, const ModelParameterMap& parameters);

    StateValue evaluate(const std::string& property_name, const PropertiesStatePoint& state_point) const;

    const std::vector<std::string>& required_states(const std::string& property_name, const std::string& model_name) const;

private:
    YoungModulusModelManager young_modulus_model_manager_;
    PoissonRatioModelManager poisson_ratio_model_manager_;
    ShearModulusModelManager shear_modulus_model_manager_;
    ElasticityTensorModelManager elasticity_tensor_model_manager_;
};

}
