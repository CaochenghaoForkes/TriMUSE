#include "properties/assembly/homogenized_panama_opf/homogenized_panama_opf_model_manager.hpp"
#include "properties/assembly/homogenized_panama_opf/panama.hpp"

namespace trimuse
{

HomogenizedPanamaOpfModel::~HomogenizedPanamaOpfModel() = default;

void HomogenizedPanamaOpfModelManager::register_models()
{
    registry_.register_model<HomogenizedPanamaOpf>("common.panama", {"assembly.homogenized_panama_opf", {"temperature", true}, {"precomputed.initial_domain_quadrature_weight", true}, "external.time", "external.time_step"});
}

void HomogenizedPanamaOpfModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

VectorValue HomogenizedPanamaOpfModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<RequiredState>& HomogenizedPanamaOpfModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
