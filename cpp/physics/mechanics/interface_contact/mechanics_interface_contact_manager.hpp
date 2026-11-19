#pragma once

#include <map>

#include "common/model_registry.hpp"
#include "physics/mechanics/interface_contact/mechanics_interface_contact_model.hpp"

namespace trimuse
{

class FemSystem;

class MechanicsInterfaceContactManager // 按材料界面选择模型，并向外部持有的约束器登记连接关系
{
public:
    MechanicsInterfaceContactManager(FemSystem& fem, DofConstraints& constraints, const std::vector<MechanicsInterfaceConfig>& contact_configs = {}); // 未配置的界面默认完美粘结，全部连接完成后由组装层统一 finalize

private:
    ModelRegistry<MechanicsInterfaceContactModel> registry_; // 模型名称 -> 创建模型的注册记录
    std::map<int, std::unique_ptr<MechanicsInterfaceContactModel>> models_; // 界面物理标识 -> 具体模型，同一物理界面的面片共用模型
};

}
