#include "physics/radioactivity/common/radioactivity_domain_manager.hpp"
namespace trimuse
{

RadioactivityDomainManager::RadioactivityDomainManager(const FemSystem& fem)
    : fem_(fem), values_(fem.domain_quadrature_point_count(), 0.0)
{
}

std::vector<RadioactivityDomainConfig> RadioactivityDomainManager::defaults(const FemSystem& fem, const std::string& model)
{
    std::vector<RadioactivityDomainConfig> configs;
    for (int region : fem.mesh_data().region_attributes()) { configs.push_back({region, model, {}}); }

    return configs;
}

void RadioactivityDomainManager::assign(const std::vector<RadioactivityDomainConfig>& configs)
{
    for (const auto& config : configs)
    {
        const auto& entry = registry_.registration(config.model_name);
        models_.emplace(config.region_attribute, Assignment{entry.factory(config.parameters), entry.required_states});
    }
}

void RadioactivityDomainManager::update(const PropertiesFieldData& properties)
{
    for (auto& [region, assignment] : models_)
    for (int element : fem_.mesh_data().region_elements(region))
    for (std::size_t point = fem_.domain_quadrature_offset(element);
         point < fem_.domain_quadrature_offset(element + 1); ++point)
    {
        const double value = assignment.model->evaluate(properties, point);
        assignment.dirty = assignment.dirty || value != values_[point];
        values_[point] = value;
    }
}

bool RadioactivityDomainManager::coupled(const PropertiesSystem& properties, const std::string& field) const
{
    for (const auto& [region, assignment] : models_)
    for (const auto& name : assignment.required)
    {
        if (properties.coupled_with_field(name, region, field))
        {
            return true;
        }
    }
    return false;
}

void RadioactivityDomainManager::mark_coupled(PropertiesSystem& properties, const std::string& field) const
{
    for (const auto& [region, assignment] : models_)
    {
        properties.mark_coupled(field, assignment.required);
    }
}

bool RadioactivityDomainManager::required_no_change(int region) const
{
    return !models_.at(region).dirty;
}

void RadioactivityDomainManager::assembled(int region)
{
    models_.at(region).dirty = false;
}

std::vector<int> RadioactivityDomainManager::regions() const
{
    std::vector<int> result;
    for (const auto& [region, assignment] : models_)
    {
        result.push_back(region);
    }
    return result;
}

}
