#pragma once

#include "physics/common/interface/interface_context.hpp"
#include "physics/common/constraint/dof_constraints.hpp"

namespace trimuse
{

class ThermalInterfaceAssembly // 汇集界面自由度约束和双侧传热贡献
{
public:
    ThermalInterfaceAssembly(FemSystem& fem, const TrialField& trial_field, DofConstraints& constraints);

    void connect(const InterfaceContext& context); // 找出理想接触界面两侧对应的自由度，并把它们登记为同一组，后续由同一个温度未知量表示

    void add_transfer(const InterfaceContext& context, mfem::Coefficient& conductance); // 把当前界面片的传热贡献，累加到界面总矩阵 matrix_ 中

    std::unique_ptr<mfem::SparseMatrix> finalize(); // 整理并返回界面传热矩阵 K_I，没有积分型贡献时为空

private:
    const FemSystem& fem_; // 求积时提供几何对称权重

    const TrialField& trial_field_; // 试探场

    std::unique_ptr<mfem::SparseMatrix> matrix_; // 保存所有热阻界面片组装得到的界面传热矩阵 K_I，后续加入热问题的总导热矩阵 K=K_{\mathrm{域内}}+K_{\mathrm{Robin}}+K_I.

    DofConstraints& constraints_; // 借用 ThermalAssembly 的约束对象，登记理想接触两侧的自由度相等关系
};

}
