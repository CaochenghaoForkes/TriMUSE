#include "properties/assembly/internal_gas_pressure/internal_gas_pressure_model_manager.hpp"
#include "properties/assembly/internal_gas_pressure/ideal_gas_from_co_fission_gas_release.hpp"
#include "properties/assembly/internal_gas_pressure/constant.hpp"
#include "properties/assembly/internal_gas_pressure/time_series.hpp"

namespace trimuse
{

InternalGasPressureModel::~InternalGasPressureModel() = default;

void InternalGasPressureModelManager::register_models()
{
    registry_.register_model<ConstantInternalGasPressure>("common.constant");
    registry_.register_model<TimeSeriesInternalGasPressure>("common.time_series", {"external.time"});
    registry_.register_model<IdealGasFromCoFissionGasRelease>("common.ideal_gas_from_co_fission_gas_release", {{"assembly.cumulative_fission_gas_release", true}, {"assembly.cumulative_co_release", true}, {"material.general.porosity", true}, {"precomputed.initial_domain_quadrature_weight", true}, {"temperature", true}});
}

void InternalGasPressureModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double InternalGasPressureModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<RequiredState>& InternalGasPressureModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
