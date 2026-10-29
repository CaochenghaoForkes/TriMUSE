#include "physics/radioactivity/boundary/radioactivity_boundary_manager.hpp"
#include "physics/radioactivity/boundary/radioactivity_dirichlet_boundary.hpp"
#include "physics/radioactivity/boundary/radioactivity_neumann_boundary.hpp"
#include "physics/radioactivity/boundary/radioactivity_robin_boundary.hpp"

namespace trimuse
{

RadioactivityBoundaryManager::RadioactivityBoundaryManager(const std::vector<RadioactivityBoundaryConfig>& configs)
{
    registry_.register_model<RadioactivityDirichletBoundary>("dirichlet", {"external.surface_nuclide_concentration"});
    registry_.register_model<RadioactivityNeumannBoundary>("neumann", {"external.nuclide_flux"});
    registry_.register_model<RadioactivityRobinBoundary>("robin", {"derived.mass_transfer_coef", "external.environment_nuclide_concentration"});
    for (const auto& config : configs)
    {
        const auto& entry = registry_.registration(config.model_name);
        models_.emplace(config.boundary_attribute, Assignment{entry.factory(config.parameters), entry.required_states});
    }
}

void RadioactivityBoundaryManager::update(const PropertiesFieldData& properties)
{
    for (auto& [zone, assignment] : models_)
    {
        const auto value = assignment.model->evaluate(properties, zone);
        assignment.dirty[0] = assignment.dirty[0] || value.matrix != assignment.values.matrix;
        assignment.dirty[1] = assignment.dirty[1] || value.load != assignment.values.load;
        assignment.dirty[2] = assignment.dirty[2] || value.concentration != assignment.values.concentration;
        assignment.values = value;
    }
}

void RadioactivityBoundaryManager::mark_coupled(PropertiesSystem& properties, const std::string& field) const
{
    for (const auto& [zone, assignment] : models_)
    {
        properties.mark_coupled(field, assignment.required);
    }
}

bool RadioactivityBoundaryManager::coupled(const PropertiesSystem& properties, const std::string& field) const
{
    for (const auto& [zone, assignment] : models_)
    for (const auto& name : assignment.required)
    {
        if (properties.coupled_with_field(name, zone, field))
        {
            return true;
        }
    }
    return false;
}

bool RadioactivityBoundaryManager::required_no_change(int zone, RadioactivityBoundaryPart part) const
{
    return !models_.at(zone).dirty[static_cast<int>(part)];
}

void RadioactivityBoundaryManager::assembled(int zone, RadioactivityBoundaryPart part)
{
    models_.at(zone).dirty[static_cast<int>(part)] = false;
}

std::vector<int> RadioactivityBoundaryManager::zones() const
{
    std::vector<int> result;
    for (const auto& [zone, assignment] : models_)
    {
        result.push_back(zone);
    }
    return result;
}

const RadioactivityBoundaryValues& RadioactivityBoundaryManager::values(int zone) const
{
    return models_.at(zone).values;
}

bool RadioactivityBoundaryManager::essential(int zone) const
{
    return models_.at(zone).model->essential();
}

}
