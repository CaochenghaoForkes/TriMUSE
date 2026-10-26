#include "properties/external/nuclide_flux/nuclide_flux_model_manager.hpp"
#include "properties/external/nuclide_flux/constant_nuclide_flux.hpp"
#include "properties/external/nuclide_flux/time_series_nuclide_flux.hpp"

namespace trimuse
{

void NuclideFluxModelManager::register_models()
{
    registry_.register_model<ConstantNuclideFlux>("constant");
    registry_.register_model<TimeSeriesNuclideFlux>("time_series", {"time"});
}

void NuclideFluxModelManager::assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    zone_model_map_[zone_attribute] = registry_.registration(model_name).factory(parameters, value_type);
}

StateValue NuclideFluxModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return zone_model_map_.at(point.zone_attribute)->evaluate(point);
}

const std::vector<std::string>& NuclideFluxModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
