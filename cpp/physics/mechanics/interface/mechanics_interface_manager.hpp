#pragma once

#include <map>

#include "common/model_registry.hpp"
#include "physics/mechanics/interface/mechanics_interface_model.hpp"

namespace trimuse
{

class FemSystem;

class MechanicsInterfaceManager // 按材料界面选择模型，并向外部持有的约束器登记连接关系
{
public:
    MechanicsInterfaceManager(FemSystem& fem, DofConstraints& constraints, const std::vector<MechanicsInterfaceConfig>& configs = {}); // 未配置的界面默认完美粘结，全部连接完成后由组装层统一 finalize

private:
    ModelRegistry<MechanicsInterfaceModel> registry_; // 模型名称 -> 创建模型的注册记录
    std::map<int, std::unique_ptr<MechanicsInterfaceModel>> models_; // 界面物理标识 -> 具体模型，同一物理界面的面片共用模型
};

}
