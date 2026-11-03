#include "properties/external/fast_neutron_flux/fast_neutron_flux_model_manager.hpp"
#include "properties/external/fast_neutron_flux/constant_fast_neutron_flux.hpp"
#include "properties/external/fast_neutron_flux/time_series_fast_neutron_flux.hpp"

#include <stdexcept>

namespace trimuse
{

void FastNeutronFluxModelManager::register_models()
{
    registry_.register_model<ConstantFastNeutronFlux>("common.constant");
    registry_.register_model<TimeSeriesFastNeutronFlux>("common.time_series", {"external.time"});
}

void FastNeutronFluxModelManager::assign(const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    model_ = registry_.registration(model_name).factory(parameters, value_type);
}

StateValue FastNeutronFluxModelManager::evaluate(const PropertiesStatePoint& point) const
{
    if (!model_) 
    { 
        throw std::logic_error("尚未设置 fast_neutron_flux 模型"); 
    }
    return model_->evaluate(point);
}

const std::vector<std::string>& FastNeutronFluxModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
