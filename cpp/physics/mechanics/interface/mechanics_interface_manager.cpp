#include "physics/mechanics/interface/mechanics_interface_manager.hpp"
#include "physics/mechanics/interface/perfect_contact.hpp"
#include "physics/common/interface/interface_context.hpp"

namespace trimuse
{

MechanicsInterfaceManager::MechanicsInterfaceManager(FemSystem& fem, DofConstraints& constraints, const std::vector<MechanicsInterfaceConfig>& configs)
{
    registry_.register_model<MechanicsPerfectContact>("perfect_contact"); // 注册界面模型
    for (const auto& config : configs) // 安排界面模型
    {
        const auto& registration = registry_.registration(config.model_name);
        models_.emplace(config.boundary_attribute, registration.factory(config.parameters));
    }

    const auto& default_model = registry_.registration("perfect_contact");
    for (const auto& interface : fem.mesh_data().interfaces()) // 组装完美接触连接，或未来需要拓展的界面关系
    {
        auto model = models_.find(interface.boundary_attribute);
        if (model == models_.end()) // 如果该面片没安排模型，就使用完美接触模型
        {
            model = models_.emplace(interface.boundary_attribute, default_model.factory({})).first;
        }
        const InterfaceContext context(interface, fem);
        model->second->assemble(context, constraints);
    }
}

}
