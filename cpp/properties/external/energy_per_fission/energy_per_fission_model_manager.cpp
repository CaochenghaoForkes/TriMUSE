#include "properties/external/energy_per_fission/energy_per_fission_model_manager.hpp"
#include "properties/external/energy_per_fission/constant_energy_per_fission.hpp"

namespace trimuse
{

EnergyPerFissionModel::~EnergyPerFissionModel() = default;

void EnergyPerFissionModelManager::register_models()
{
    registry_.register_model<ConstantEnergyPerFission>("common.constant");
}

void EnergyPerFissionModelManager::assign(const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    model_ = registry_.registration(model_name).factory(parameters, value_type);
}

StateValue EnergyPerFissionModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return model_->evaluate(point);
}

const std::vector<RequiredState>& EnergyPerFissionModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
