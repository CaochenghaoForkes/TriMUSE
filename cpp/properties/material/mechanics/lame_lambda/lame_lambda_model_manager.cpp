#include "properties/material/mechanics/lame_lambda/lame_lambda_model_manager.hpp"
#include "properties/material/mechanics/lame_lambda/common/fixed_relation_lame_lambda.hpp"

namespace trimuse
{

void LameLambdaModelManager::register_models()
{
    registry_.register_model<FixedRelationLameLambda>("common.fixed_relation", {"material.mechanics.young_modulus", "material.mechanics.poisson_ratio"});
}

void LameLambdaModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double LameLambdaModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<std::string>& LameLambdaModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
