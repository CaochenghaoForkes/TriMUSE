#include "physics/radioactivity/interface/radioactivity_interface_manager.hpp"
#include "physics/radioactivity/interface/perfect_contact.hpp"

namespace trimuse
{

RadioactivityInterfaceManager::RadioactivityInterfaceManager(FemSystem& fem, const TrialField& trial_field,
    const std::vector<RadioactivityInterfaceConfig>& configs)
    : constraints_(fem, trial_field)
{
    registry_.register_model<RadioactivityPerfectContact>("perfect_contact");
    for (const auto& config : configs)
    {
        const auto& registration = registry_.registration(config.model_name);
        models_.emplace(config.boundary_attribute, registration.factory(config.parameters));
    }
    for (const auto& interface : fem.mesh_data().interfaces())
    {
        auto model = models_.find(interface.boundary_attribute);
        if (model == models_.end())
        {
            const auto& registration = registry_.registration("perfect_contact");
            model = models_.emplace(interface.boundary_attribute, registration.factory({})).first;
        }
        const InterfaceContext context(interface, fem);
        model->second->assemble(context, constraints_);
    }
    constraints_.finalize();
}

}
