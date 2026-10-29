#include "physics/thermal/interface/thermal_interface_assembly.hpp"

namespace trimuse
{

ThermalInterfaceAssembly::ThermalInterfaceAssembly(FemSystem& fem, const TrialField& field)
    : field_(field), integrator_(fem), constraints_(std::make_unique<DofConstraints>(fem, field)) // 传入试探场；有限元系统用于初始化传热积分器和公共约束对象
{
}

void ThermalInterfaceAssembly::connect(const ThermalInterfaceContext& context) // 找出理想接触界面两侧对应的自由度，并把它们登记为同一组，后续由同一个温度未知量表示
{
    constraints_->connect(context.interface());
}

void ThermalInterfaceAssembly::add_transfer(const ThermalInterfaceContext& context, mfem::Coefficient& conductance) // 把当前界面片的传热贡献，累加到界面总矩阵 matrix_ 中
{
    if (!matrix_) // 判断矩阵指针是否为空 第一次处理热阻界面时，还没有创建矩阵
    { 
        matrix_ = std::make_unique<mfem::SparseMatrix>(field_.size()); // 创建一个方形稀疏矩阵，行列数均等于温度场的完整自由度总数
    }
    integrator_.assemble(context, conductance, *matrix_); // 调用界面积分器计算当前面片的局部矩阵 并累加到 matrix_ 中
}

ThermalInterfaceAssembly::Operators ThermalInterfaceAssembly::finalize() // 完成界面矩阵的组装，并返回约束映射 P 和界面传热矩阵 K_I
{
    if (matrix_) 
    { 
        matrix_->Finalize(); // 如果存在热阻界面矩阵，就调用 MFEM 的 Finalize()，将可逐项累加的稀疏矩阵整理为后续矩阵运算使用的 CSR 存储格式
    }
    constraints_->finalize();
    return {std::move(constraints_), std::move(matrix_)};
}

}
