#include "properties/external/surface_traction/surface_traction_model_manager.hpp"
#include "properties/external/surface_traction/constant_surface_traction.hpp"
#include "properties/external/surface_traction/time_series_surface_traction.hpp"

namespace trimuse
{

SurfaceTractionModel::~SurfaceTractionModel() = default;

void SurfaceTractionModelManager::register_models()
{
    registry_.register_model<ConstantSurfaceTraction>("common.constant");
    registry_.register_model<TimeSeriesSurfaceTraction>("common.time_series", {"external.time"});
}

void SurfaceTractionModelManager::assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    zone_model_map_[zone_attribute] = registry_.registration(model_name).factory(parameters);
}

VectorValue SurfaceTractionModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return zone_model_map_.at(point.zone_attribute)->evaluate(point);
}

const std::vector<RequiredState>& SurfaceTractionModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
