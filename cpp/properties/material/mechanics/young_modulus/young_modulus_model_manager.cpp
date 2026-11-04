#include "properties/material/mechanics/young_modulus/young_modulus_model_manager.hpp"
#include "properties/material/mechanics/young_modulus/common/constant_young_modulus.hpp"
#include "properties/material/mechanics/young_modulus/common/time_series_young_modulus.hpp"

namespace trimuse
{

void YoungModulusModelManager::register_models()
{
    registry_.register_model<ConstantYoungModulus>("common.constant");
    registry_.register_model<TimeSeriesYoungModulus>("common.time_series", {"external.time"});
}

void YoungModulusModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double YoungModulusModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<std::string>& YoungModulusModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
