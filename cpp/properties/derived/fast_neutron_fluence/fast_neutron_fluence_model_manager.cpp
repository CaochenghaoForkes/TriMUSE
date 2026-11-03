#include "properties/derived/fast_neutron_fluence/fast_neutron_fluence_model_manager.hpp"
#include "properties/derived/fast_neutron_fluence/constant_fast_neutron_fluence.hpp"
#include "properties/derived/fast_neutron_fluence/time_series_fast_neutron_fluence.hpp"
#include "properties/derived/fast_neutron_fluence/accumulate_from_fast_neutron_flux.hpp"

#include <stdexcept>

namespace trimuse
{

void FastNeutronFluenceModelManager::register_models()
{
    registry_.register_model<ConstantFastNeutronFluence>("common.constant");
    registry_.register_model<TimeSeriesFastNeutronFluence>("common.time_series", {"external.time"});
    registry_.register_model<AccumulateFromFastNeutronFlux>("common.accumulate_from_fast_neutron_flux", {"derived.fast_neutron_fluence", "external.fast_neutron_flux", "external.time_step"});
}

void FastNeutronFluenceModelManager::assign(const std::string& model_name, const ModelParameterMap& parameters)
{
    model_ = registry_.registration(model_name).factory(parameters);
}

StateValue FastNeutronFluenceModelManager::evaluate(const PropertiesStatePoint& point) const
{
    if (!model_) 
    { 
        throw std::logic_error("尚未设置 fast_neutron_fluence 模型"); 
    }
    return model_->evaluate(point);
}

const std::vector<std::string>& FastNeutronFluenceModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
