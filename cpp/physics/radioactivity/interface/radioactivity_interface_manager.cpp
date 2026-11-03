#include "physics/radioactivity/interface/radioactivity_interface_manager.hpp"
#include "physics/radioactivity/interface/perfect_contact.hpp"

namespace trimuse
{

RadioactivityInterfaceManager::RadioactivityInterfaceManager(FemSystem& fem, const TrialField& trial_field,
    const std::vector<RadioactivityInterfaceConfig>& configs) // 按界面标识选择模型，未配置的界面默认完美接触
    : constraints_(fem, trial_field)
{
    registry_.register_model<RadioactivityPerfectContact>("perfect_contact");
    for (const auto& config : configs)
    {
        const auto& registration = registry_.registration(config.model_name);
        models_.emplace(config.boundary_attribute, registration.factory(config.parameters));
    }
    for (const auto& interface : fem.mesh_data().interfaces()) // 遍历原网格中的每片双侧材料界面
    {
        auto model = models_.find(interface.boundary_attribute);
        if (model == models_.end())
        {
            const auto& registration = registry_.registration("perfect_contact");
            model = models_.emplace(interface.boundary_attribute, registration.factory({})).first;
        }
        const InterfaceContext context(interface, fem);
        model->second->assemble(context, constraints_); // 当前模型向约束器登记两侧自由度连接
    }
    constraints_.finalize(); // 全部界面处理完后一次性建立 P，后续时间步重复使用
}

}
