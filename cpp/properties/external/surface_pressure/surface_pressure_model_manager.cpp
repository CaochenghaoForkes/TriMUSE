#include "properties/external/surface_pressure/surface_pressure_model_manager.hpp"
#include "properties/external/surface_pressure/constant_surface_pressure.hpp"
#include "properties/external/surface_pressure/time_series_surface_pressure.hpp"

namespace trimuse
{

void SurfacePressureModelManager::register_models()
{
    registry_.register_model<ConstantSurfacePressure>("common.constant");
    registry_.register_model<TimeSeriesSurfacePressure>("common.time_series", {"external.time"});
}

void SurfacePressureModelManager::assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    zone_model_map_[zone_attribute] = registry_.registration(model_name).factory(parameters);
}

double SurfacePressureModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return zone_model_map_.at(point.zone_attribute)->evaluate(point);
}

const std::vector<std::string>& SurfacePressureModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
