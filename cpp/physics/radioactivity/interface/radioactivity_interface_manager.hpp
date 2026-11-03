#pragma once

#include "common/model_registry.hpp"
#include "physics/radioactivity/interface/radioactivity_interface_model.hpp"

namespace trimuse
{

class RadioactivityInterfaceManager // 为每片内部界面选模型，并持有最终自由度约束矩阵 P
{
public:
    RadioactivityInterfaceManager(FemSystem& fem, const TrialField& trial_field,
        const std::vector<RadioactivityInterfaceConfig>& configs);

    const DofConstraints& constraints() const { return constraints_; } // 提供约化、恢复等约束操作
    const mfem::SparseMatrix& prolongation() const { return constraints_.prolongation(); } // P：独立浓度自由度 -> 完整自由度

private:
    ModelRegistry<RadioactivityInterfaceModel> registry_; // 界面模型注册表
    std::map<int, std::unique_ptr<RadioactivityInterfaceModel>> models_; // 界面物理标识 -> 具体模型
    DofConstraints constraints_; // 收集完美接触相等关系并生成固定的 P
};

}
