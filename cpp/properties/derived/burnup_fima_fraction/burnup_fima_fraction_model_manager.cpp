#include "properties/derived/burnup_fima_fraction/burnup_fima_fraction_model_manager.hpp"
#include "properties/derived/burnup_fima_fraction/constant_burnup_fima_fraction.hpp"
#include "properties/derived/burnup_fima_fraction/time_series_burnup_fima_fraction.hpp"

namespace trimuse
{

BurnupFimaFractionModel::~BurnupFimaFractionModel() = default;

void BurnupFimaFractionModelManager::register_models()
{
    registry_.register_model<ConstantBurnupFimaFraction>("common.constant");
    registry_.register_model<TimeSeriesBurnupFimaFraction>("common.time_series", {"external.time"});
}

void BurnupFimaFractionModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

StateValue BurnupFimaFractionModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<std::string>& BurnupFimaFractionModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
