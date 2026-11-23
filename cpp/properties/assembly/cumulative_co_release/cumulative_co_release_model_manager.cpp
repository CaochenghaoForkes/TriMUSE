#include "properties/assembly/cumulative_co_release/cumulative_co_release_model_manager.hpp"
#include "properties/assembly/cumulative_co_release/from_local_panama_opf.hpp"
#include "properties/assembly/cumulative_co_release/from_homogenized_panama_opf.hpp"

namespace trimuse
{

CumulativeCoReleaseModel::~CumulativeCoReleaseModel() = default;

void CumulativeCoReleaseModelManager::register_models()
{
    registry_.register_model<FromHomogenizedPanamaOpf>("common.from_homogenized_panama_opf", {"assembly.homogenized_panama_opf", "external.initial_heavy_metal_inventory", "derived.burnup_fima_fraction"});
    registry_.register_model<FromLocalPanamaOpf>("common.from_local_panama_opf", {{"assembly.local_panama_opf", true}, {"precomputed.initial_domain_quadrature_weight", true}, "external.initial_heavy_metal_inventory", "derived.burnup_fima_fraction"});
}

void CumulativeCoReleaseModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double CumulativeCoReleaseModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<RequiredState>& CumulativeCoReleaseModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
