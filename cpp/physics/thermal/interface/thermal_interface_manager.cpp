#include "physics/thermal/interface/thermal_interface_manager.hpp"
#include "physics/thermal/interface/thermal_interface_assembly.hpp"
#include "physics/thermal/interface/perfect_contact.hpp"
#include "physics/thermal/interface/uniform_from_constant_resistance.hpp"
#include "physics/thermal/interface/local_from_constant_resistance.hpp"

#include <stdexcept>

namespace trimuse
{

ThermalInterfaceManager::ThermalInterfaceManager(FemSystem& fem, TrialField& trial_field, DofConstraints& constraints, const std::vector<ThermalInterfaceConfig>& configs)
{
    registry_.register_model<ThermalPerfectContact>("perfect_contact");
    registry_.register_model<ThermalUniformFromConstantResistance>("uniform_from_constant_resistance");
    registry_.register_model<ThermalLocalFromConstantResistance>("local_from_constant_resistance");

    for (const auto& config : configs)
    {
        const auto& registration = registry_.registration(config.model_name);
        auto model = registration.factory(config.parameters);
        if (!trial_field.regional() && model->requires_region_field())
        {
            throw std::invalid_argument("所选热界面模型需要分域温度场: " + config.model_name);
        }
        models_.emplace(config.boundary_attribute, std::move(model));
    }

    ThermalInterfaceAssembly assembly(fem, trial_field, constraints); // 汇集界面自由度约束和双侧传热贡献
    for (const auto& interface : fem.mesh_data().interfaces()) // 遍历原网格中连接不同 region 的内部界面片
    {
        auto model = models_.find(interface.boundary_attribute); // 根据当前界面片的物理属性编号，查找它使用的模型
        if (model == models_.end()) // 如果尚未配置模型 则默认使用理想接触
        {
            const auto& registration = registry_.registration("perfect_contact");
            model = models_.emplace(interface.boundary_attribute, registration.factory({})).first; // 取得理想接触模型的注册记录，包括创建模型的工厂函数
        }
        const InterfaceContext context(interface, fem); // 为当前面片创建公共几何访问对象，提供两侧单元、几何变换和求积规则
        model->second->assemble(context, assembly); // 调用具体模型 理想接触: 登记两侧自由度连接 恒定热阻: 将该面片的传热贡献累加到界面矩阵
    }
    matrix_ = assembly.finalize(); // 整理界面矩阵并接管所有权，不复制矩阵数值；自由度约束由 ThermalAssembly 统一完成并保存
}

}
