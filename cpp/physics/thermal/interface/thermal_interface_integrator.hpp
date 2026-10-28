#pragma once

#include "physics/thermal/interface/thermal_interface_context.hpp"

namespace trimuse
{

class ThermalInterfaceIntegrator // 将 q=h(T_a-T_b) 积分为双侧传热矩阵; h 可以在各界面积分点求值
{
public:
    explicit ThermalInterfaceIntegrator(const FemSystem& fem) : fem_(fem) {}

    void assemble(const ThermalInterfaceContext& context, mfem::Coefficient& conductance, mfem::SparseMatrix& matrix) const; // 把当前界面片的有限热阻传热关系，积分成耦合两侧单元的局部矩阵，再累加到整体界面矩阵中 context: 当前界面片及其双侧场与几何信息 conductance: 界面传热系数 h=1/R 的求值对象 matrix: 接收组装结果的整体稀疏矩阵使用完整自由度编号 (整个热界面模块共用的一张矩阵，保存所有热阻界面片的传热贡献 后续会传给热问题最终的系统矩阵)

private:
    const FemSystem& fem_; // 有限元空间
};

}
