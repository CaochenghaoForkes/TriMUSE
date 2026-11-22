#include "properties/derived/fission_gas_production_rate/fission_gas_production_rate_model_manager.hpp"
#include "properties/derived/fission_gas_production_rate/constant_fission_gas_production_rate.hpp"
#include "properties/derived/fission_gas_production_rate/time_series_fission_gas_production_rate.hpp"
#include "properties/derived/fission_gas_production_rate/kernel/bison_2021_kernel_fission_gas_production_rate.hpp"

namespace trimuse
{

FissionGasProductionRateModel::~FissionGasProductionRateModel() = default;

void FissionGasProductionRateModelManager::register_models()
{
    registry_.register_model<ConstantFissionGasProductionRate>("common.constant");
    registry_.register_model<TimeSeriesFissionGasProductionRate>("common.time_series", {"external.time"});
    registry_.register_model<Bison2021KernelFissionGasProductionRate>("kernel.bison_2021", {"derived.average_volumetric_fission_rate", "derived.region_volume"});
}

void FissionGasProductionRateModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

StateValue FissionGasProductionRateModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<RequiredState>& FissionGasProductionRateModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
