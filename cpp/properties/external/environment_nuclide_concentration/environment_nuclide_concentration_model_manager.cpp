#include "properties/external/environment_nuclide_concentration/environment_nuclide_concentration_model_manager.hpp"
#include "properties/external/environment_nuclide_concentration/constant_environment_nuclide_concentration.hpp"
#include "properties/external/environment_nuclide_concentration/time_series_environment_nuclide_concentration.hpp"

namespace trimuse
{

void EnvironmentNuclideConcentrationModelManager::register_models()
{
    registry_.register_model<ConstantEnvironmentNuclideConcentration>("common.constant");
    registry_.register_model<TimeSeriesEnvironmentNuclideConcentration>("common.time_series", {"external.time"});
}

void EnvironmentNuclideConcentrationModelManager::assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    zone_model_map_[zone_attribute] = registry_.registration(model_name).factory(parameters, value_type);
}

StateValue EnvironmentNuclideConcentrationModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return zone_model_map_.at(point.zone_attribute)->evaluate(point);
}

const std::vector<std::string>& EnvironmentNuclideConcentrationModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
