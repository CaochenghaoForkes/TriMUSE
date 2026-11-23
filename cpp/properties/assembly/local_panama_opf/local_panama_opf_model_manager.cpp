#include "properties/assembly/local_panama_opf/local_panama_opf_model_manager.hpp"
#include "properties/assembly/local_panama_opf/panama.hpp"

namespace trimuse
{

LocalPanamaOpfModel::~LocalPanamaOpfModel() = default;

void LocalPanamaOpfModelManager::register_models()
{
    registry_.register_model<PanamaOpf>("common.panama", {"assembly.local_panama_opf", "temperature", "external.time", "external.time_step"});
}

void LocalPanamaOpfModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

VectorValue LocalPanamaOpfModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<RequiredState>& LocalPanamaOpfModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
