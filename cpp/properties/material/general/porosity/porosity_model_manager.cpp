#include "properties/material/general/porosity/porosity_model_manager.hpp"

#include "properties/material/general/porosity/common/constant_porosity.hpp"
#include "properties/material/general/porosity/common/time_series_porosity.hpp"

namespace trimuse
{

void PorosityModelManager::register_models()
{
    registry_.register_model<ConstantPorosity>("common.constant");
    registry_.register_model<TimeSeriesPorosity>("common.time_series", {"time"});
}

void PorosityModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double PorosityModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<std::string>& PorosityModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
