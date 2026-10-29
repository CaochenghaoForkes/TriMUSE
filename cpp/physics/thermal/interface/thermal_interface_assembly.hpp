#pragma once

#include "physics/thermal/interface/thermal_interface_context.hpp"
#include "physics/common/constraint/dof_constraints.hpp"
#include "physics/thermal/interface/thermal_interface_integrator.hpp"

namespace trimuse
{

class ThermalInterfaceAssembly // 汇集界面自由度约束和双侧传热贡献
{
public:
    struct Operators
    {
        std::unique_ptr<DofConstraints> constraints; // 公共自由度约束及映射 P
        std::unique_ptr<mfem::SparseMatrix> matrix; // 没有积分型贡献时为空
    };

    ThermalInterfaceAssembly(FemSystem& fem, const TrialField& field);

    void connect(const ThermalInterfaceContext& context); // 找出理想接触界面两侧对应的自由度，并把它们登记为同一组，后续由同一个温度未知量表示

    void add_transfer(const ThermalInterfaceContext& context, mfem::Coefficient& conductance); // 把当前界面片的传热贡献，累加到界面总矩阵 matrix_ 中

    Operators finalize(); // 完成界面矩阵的组装，并返回约束映射 P 和界面传热矩阵 K_I

private:
    const TrialField& field_; // 试探场

    ThermalInterfaceIntegrator integrator_; // 将 q=h(T_a-T_b) 积分为双侧传热矩阵; h 可以在各界面积分点求值

    std::unique_ptr<mfem::SparseMatrix> matrix_; // 保存所有热阻界面片组装得到的界面传热矩阵 K_I，后续加入热问题的总导热矩阵 K=K_{\mathrm{域内}}+K_{\mathrm{Robin}}+K_I.

    std::unique_ptr<DofConstraints> constraints_; // 登记两侧相等关系并构建 P，完成组装后交给管理器长期保存
};

}
