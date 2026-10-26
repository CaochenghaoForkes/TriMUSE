#include "properties/external/surface_nuclide_concentration/surface_nuclide_concentration_model_manager.hpp"
#include "properties/external/surface_nuclide_concentration/constant_surface_nuclide_concentration.hpp"
#include "properties/external/surface_nuclide_concentration/time_series_surface_nuclide_concentration.hpp"

namespace trimuse
{

void SurfaceNuclideConcentrationModelManager::register_models()
{
    registry_.register_model<ConstantSurfaceNuclideConcentration>("common.constant");
    registry_.register_model<TimeSeriesSurfaceNuclideConcentration>("common.time_series", {"external.time"});
}

void SurfaceNuclideConcentrationModelManager::assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    zone_model_map_[zone_attribute] = registry_.registration(model_name).factory(parameters, value_type);
}

StateValue SurfaceNuclideConcentrationModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return zone_model_map_.at(point.zone_attribute)->evaluate(point);
}

const std::vector<std::string>& SurfaceNuclideConcentrationModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
