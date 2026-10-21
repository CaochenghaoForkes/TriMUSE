#include "properties/external/environment_nuclide_concentration/environment_nuclide_concentration_model_manager.hpp"

namespace trimuse
{

void EnvironmentNuclideConcentrationModelManager::register_models()
{
    common_model_manager_.register_models();
}

void EnvironmentNuclideConcentrationModelManager::assign(int zone_attribute, const std::string& model_name, const ModelParameterMap& parameters, const std::string& value_type)
{
    zone_model_map_.insert_or_assign(zone_attribute, common_model_manager_.create(model_name, parameters, value_type));
}

StateValue EnvironmentNuclideConcentrationModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return std::visit([&point](const auto& model)
    {
        return model.evaluate(point);
    }, zone_model_map_.at(point.zone_attribute));
}

const std::vector<std::string>& EnvironmentNuclideConcentrationModelManager::required_states(const std::string& model_name) const
{
    return common_model_manager_.required_states(model_name);
}

}
