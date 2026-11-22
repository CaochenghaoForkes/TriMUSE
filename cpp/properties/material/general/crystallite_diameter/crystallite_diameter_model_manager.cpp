#include "properties/material/general/crystallite_diameter/crystallite_diameter_model_manager.hpp"
#include "properties/material/general/crystallite_diameter/common/constant_crystallite_diameter.hpp"

namespace trimuse
{

CrystalliteDiameterModel::~CrystalliteDiameterModel() = default;

void CrystalliteDiameterModelManager::register_models()
{
    registry_.register_model<ConstantCrystalliteDiameter>("common.constant");
}

void CrystalliteDiameterModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

double CrystalliteDiameterModelManager::evaluate(const PropertiesStatePoint& state_point) const
{
    return region_model_map_.at(state_point.region_attribute)->evaluate(state_point);
}

const std::vector<RequiredState>& CrystalliteDiameterModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
