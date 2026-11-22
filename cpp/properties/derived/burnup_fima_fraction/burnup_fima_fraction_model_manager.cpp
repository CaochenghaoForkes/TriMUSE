#include "properties/derived/burnup_fima_fraction/burnup_fima_fraction_model_manager.hpp"
#include "properties/derived/burnup_fima_fraction/constant_burnup_fima_fraction.hpp"
#include "properties/derived/burnup_fima_fraction/time_series_burnup_fima_fraction.hpp"
#include "properties/derived/burnup_fima_fraction/accumulate_from_fission_rate.hpp"

namespace trimuse
{

BurnupFimaFractionModel::~BurnupFimaFractionModel() = default;

void BurnupFimaFractionModelManager::register_models()
{
    registry_.register_model<ConstantBurnupFimaFraction>("common.constant");
    registry_.register_model<TimeSeriesBurnupFimaFraction>("common.time_series", {"external.time"});
    registry_.register_model<AccumulateFromFissionRate>("common.accumulate_from_fission_rate", {"derived.burnup_fima_fraction", "derived.average_volumetric_fission_rate", "derived.region_volume", "external.initial_heavy_metal_inventory", "external.time_step"});
}

void BurnupFimaFractionModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

StateValue BurnupFimaFractionModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<RequiredState>& BurnupFimaFractionModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
