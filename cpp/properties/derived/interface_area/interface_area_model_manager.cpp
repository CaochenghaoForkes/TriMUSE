#include "properties/derived/interface_area/interface_area_model_manager.hpp"
#include "properties/derived/interface_area/undeformed.hpp"

namespace trimuse
{

InterfaceAreaModel::~InterfaceAreaModel() = default;

void InterfaceAreaModelManager::register_models()
{
    registry_.register_model<UndeformedInterfaceArea>("common.undeformed", {"precomputed.initial_interface_area"});
}

void InterfaceAreaModelManager::assign(int interface_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    interface_model_map_[interface_attribute] = registry_.registration(model_name).factory(parameters);
}

StateValue InterfaceAreaModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return interface_model_map_.at(point.interface_attribute)->evaluate(point);
}

const std::vector<std::string>& InterfaceAreaModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
