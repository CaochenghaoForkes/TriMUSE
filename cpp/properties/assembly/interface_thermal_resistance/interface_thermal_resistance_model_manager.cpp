#include "properties/assembly/interface_thermal_resistance/interface_thermal_resistance_model_manager.hpp"
#include "properties/assembly/interface_thermal_resistance/local_from_constant_value.hpp"
#include "properties/assembly/interface_thermal_resistance/uniform_from_constant_value.hpp"

namespace trimuse
{

InterfaceThermalResistance::~InterfaceThermalResistance() = default;

void InterfaceThermalResistanceModelManager::register_models()
{
    registry_.register_model<UniformInterfaceThermalResistance>("common.uniform_from_constant_value");
    registry_.register_model<LocalInterfaceThermalResistance>("common.local_from_constant_value", {"precomputed.geometry_center"});
}

void InterfaceThermalResistanceModelManager::assign(int attribute, const std::string& model, const ModelParameterMap& parameters)
{
    interface_models_[attribute] = registry_.registration(model).factory(parameters);
}

const std::vector<RequiredState>& InterfaceThermalResistanceModelManager::required_states(const std::string& model) const
{
    return registry_.registration(model).required_states;
}

std::optional<double> InterfaceThermalResistanceModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return interface_models_.at(point.interface_attribute)->evaluate(point);
}

}
