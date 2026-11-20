#include "properties/derived/domain_quadrature_weight/domain_quadrature_weight_model_manager.hpp"
#include "properties/derived/domain_quadrature_weight/undeformed.hpp"

namespace trimuse
{

DomainQuadratureWeightModel::~DomainQuadratureWeightModel() = default;

void DomainQuadratureWeightModelManager::register_models()
{
    registry_.register_model<UndeformedDomainQuadratureWeight>("common.undeformed", {"precomputed.initial_domain_quadrature_weight"});
}

void DomainQuadratureWeightModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

StateValue DomainQuadratureWeightModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<std::string>& DomainQuadratureWeightModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
