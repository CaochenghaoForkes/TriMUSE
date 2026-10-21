#include "properties/material/thermal/specific_heat/specific_heat_model_manager.hpp"
#include "properties/common/common_model_adapter.hpp"

#include "properties/material/thermal/specific_heat/kernel/fink_2000_uo2_specific_heat.hpp"
#include "properties/material/thermal/specific_heat/sic/bison_2021_sic_specific_heat.hpp"

namespace trimuse
{

void SpecificHeatModelManager::register_models()
{
    register_common_models<SpecificHeatModel>(registry_);
    registry_.register_model<Fink2000Uo2SpecificHeat>("kernel.fink_2000_uo2", {"temperature"});
    registry_.register_model<Bison2021SicSpecificHeat>("sic.bison_2021", {"temperature"});
}

void SpecificHeatModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double SpecificHeatModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<std::string>& SpecificHeatModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
