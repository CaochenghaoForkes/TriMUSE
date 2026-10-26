#include "properties/external/burnup_fima_fraction/burnup_fima_fraction_model_manager.hpp"
#include "properties/external/burnup_fima_fraction/constant_burnup_fima_fraction.hpp"
#include "properties/external/burnup_fima_fraction/time_series_burnup_fima_fraction.hpp"

#include <stdexcept>

namespace trimuse
{

void BurnupFimaFractionModelManager::register_models()
{
    registry_.register_model<ConstantBurnupFimaFraction>("constant");
    registry_.register_model<TimeSeriesBurnupFimaFraction>("time_series", {"time"});
}

void BurnupFimaFractionModelManager::assign(const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    model_ = registry_.registration(model_name).factory(parameters, value_type);
}

StateValue BurnupFimaFractionModelManager::evaluate(const PropertiesStatePoint& point) const
{
    if (!model_) { throw std::logic_error("尚未设置 burnup_fima_fraction 模型"); }
    return model_->evaluate(point);
}

const std::vector<std::string>& BurnupFimaFractionModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
