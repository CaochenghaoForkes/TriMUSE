#include "properties/external/surface_displacement/surface_displacement_model_manager.hpp"
#include "properties/external/surface_displacement/constant_surface_displacement.hpp"
#include "properties/external/surface_displacement/time_series_surface_displacement.hpp"

namespace trimuse
{

SurfaceDisplacementModel::~SurfaceDisplacementModel() = default;

void SurfaceDisplacementModelManager::register_models()
{
    registry_.register_model<ConstantSurfaceDisplacement>("common.constant");
    registry_.register_model<TimeSeriesSurfaceDisplacement>("common.time_series", {"external.time"});
}

void SurfaceDisplacementModelManager::assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    zone_model_map_[zone_attribute] = registry_.registration(model_name).factory(parameters);
}

VectorValue SurfaceDisplacementModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return zone_model_map_.at(point.zone_attribute)->evaluate(point);
}

const std::vector<std::string>& SurfaceDisplacementModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
