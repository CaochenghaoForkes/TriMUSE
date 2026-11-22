#include "properties/derived/irradiation_time/irradiation_time_model_manager.hpp"
#include "properties/derived/irradiation_time/accumulate.hpp"
#include "properties/derived/irradiation_time/accumulate_from_fast_neutron_flux.hpp"

namespace trimuse
{

IrradiationTimeModel::~IrradiationTimeModel() = default;

void IrradiationTimeModelManager::register_models()
{
    registry_.register_model<AccumulateIrradiationTime>("common.accumulate", {"derived.irradiation_time", "external.time_step"});
    registry_.register_model<AccumulateIrradiationTimeFromFastNeutronFlux>("common.accumulate_from_fast_neutron_flux", {"derived.irradiation_time", "external.fast_neutron_flux", "external.time_step"});
}

void IrradiationTimeModelManager::assign(const std::string& model_name, const ModelParameterMap& parameters)
{
    model_ = registry_.registration(model_name).factory(parameters);
}

StateValue IrradiationTimeModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return model_->evaluate(point);
}

const std::vector<RequiredState>& IrradiationTimeModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
