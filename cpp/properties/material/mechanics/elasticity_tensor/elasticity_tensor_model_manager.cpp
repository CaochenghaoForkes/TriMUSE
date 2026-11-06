#include "properties/material/mechanics/elasticity_tensor/elasticity_tensor_model_manager.hpp"
#include "properties/material/mechanics/elasticity_tensor/common/from_engineering_constants_elasticity_tensor.hpp"

namespace trimuse
{

void ElasticityTensorModelManager::register_models()
{
    registry_.register_model<FromEngineeringConstantsElasticityTensor>(
        "common.from_engineering_constants",
        {"material.mechanics.young_modulus", "material.mechanics.poisson_ratio", "material.mechanics.shear_modulus",
         "precomputed.tensor_transform", "precomputed.tensor_transform_inverse"});
}

void ElasticityTensorModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

ElasticityTensorValue ElasticityTensorModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<std::string>& ElasticityTensorModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
