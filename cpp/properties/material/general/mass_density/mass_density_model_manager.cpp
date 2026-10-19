#include "properties/material/general/mass_density/mass_density_model_manager.hpp"

#include "properties/material/general/mass_density/common/constant_mass_density.hpp"
#include "properties/material/general/mass_density/common/time_series_mass_density.hpp"

namespace trimuse
{

void MassDensityModelManager::register_models()
{
    registry_.register_model<ConstantMassDensity>("common.constant");
    registry_.register_model<TimeSeriesMassDensity>("common.time_series", {"time"});
}

void MassDensityModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double MassDensityModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<std::string>& MassDensityModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
