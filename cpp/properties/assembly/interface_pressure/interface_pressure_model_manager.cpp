#include "properties/assembly/interface_pressure/interface_pressure_model_manager.hpp"
#include "properties/assembly/interface_pressure/uniform_from_constant_value.hpp"
#include "properties/assembly/interface_pressure/local_from_constant_value.hpp"

namespace trimuse
{

void InterfacePressureModelManager::register_models()
{
    registry_.register_model<UniformInterfacePressure>("common.uniform_from_constant_value");
    registry_.register_model<LocalInterfacePressure>("common.local_from_constant_value", {"precomputed.geometry_center"});
}

void InterfacePressureModelManager::assign(int attribute, const std::string& model, const ModelParameterMap& parameters)
{
    interface_models_[attribute] = registry_.registration(model).factory(parameters);
}

const std::vector<std::string>& InterfacePressureModelManager::required_states(const std::string& model) const
{
    return registry_.registration(model).required_states;
}

double InterfacePressureModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return interface_models_.at(point.interface_attribute)->evaluate(point);
}

}
