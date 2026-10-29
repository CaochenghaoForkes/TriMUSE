#pragma once

#include <map>
#include <memory>

#include "common/model_registry.hpp"
#include "physics/thermal/interface/thermal_interface_model.hpp"
#include "physics/common/constraint/dof_constraints.hpp"

namespace trimuse
{

class ThermalInterfaceManager
{
public:

    ThermalInterfaceManager(FemSystem& fem, TrialField& field, const std::vector<ThermalInterfaceConfig>& configs);

    const DofConstraints& constraints() const { return *constraints_; }

    const mfem::SparseMatrix& prolongation() const { return constraints_->prolongation(); }

    const mfem::SparseMatrix* matrix() const { return matrix_.get(); }

private:
    ModelRegistry<ThermalInterfaceModel> registry_; // 界面模型注册表 模型名称 -> 创建模型的工厂函数及注册信息

    std::map<int, std::unique_ptr<ThermalInterfaceModel>> models_; // 界面物理属性编号 -> 该界面使用的具体模型对象 一个物理界面可以包含多个网格面片，这些面片共用对应的模型对象

    std::unique_ptr<DofConstraints> constraints_; // 公共自由度约束，持有映射矩阵 P

    std::unique_ptr<mfem::SparseMatrix> matrix_; // 所有热阻界面片贡献累加形成的界面传热矩阵 KI
};

}
