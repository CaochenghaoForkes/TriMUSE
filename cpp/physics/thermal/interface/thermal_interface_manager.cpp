#include "physics/thermal/interface/thermal_interface_manager.hpp"
#include "physics/thermal/interface/thermal_interface_assembly.hpp"
#include "physics/thermal/interface/perfect_contact.hpp"
#include "physics/thermal/interface/insulated.hpp"

#include <stdexcept>

namespace trimuse
{

ThermalInterfaceManager::ThermalInterfaceManager(FemSystem& fem, ThermalField& field,
    const std::vector<ThermalInterfaceConfig>& configs)
{
    registry_.register_model<ThermalPerfectContact>("perfect_contact");
    registry_.register_model<ThermalInsulatedInterface>("insulated");
    for (const auto& config : configs)
    {
        const auto& registration = registry_.registration(config.model_name);
        auto model = registration.factory(config.parameters);
        if (!field.regional() && model->requires_region_field())
        {
            throw std::invalid_argument("所选热界面模型需要分域温度场: " + config.model_name);
        }
        models_.emplace(config.boundary_attribute, std::move(model));
    }

    ThermalInterfaceAssembly assembly(fem, field);
    for (const auto& interface : fem.mesh_data().interfaces())
    {
        auto model = models_.find(interface.boundary_attribute);
        if (model == models_.end())
        {
            const auto& registration = registry_.registration("perfect_contact");
            model = models_.emplace(interface.boundary_attribute, registration.factory({})).first;
        }
        const ThermalInterfaceContext context(interface, fem, field);
        model->second->assemble(context, assembly);
    }
    prolongation_ = assembly.finalize();
}

}
