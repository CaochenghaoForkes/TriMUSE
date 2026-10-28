#pragma once

#include "physics/thermal/interface/thermal_interface_context.hpp"
#include "physics/thermal/interface/thermal_interface_integrator.hpp"

namespace trimuse
{

class ThermalInterfaceAssembly // 汇集界面自由度约束和双侧传热贡献
{
public:
    struct Operators
    {
        std::unique_ptr<mfem::SparseMatrix> prolongation;
        std::unique_ptr<mfem::SparseMatrix> matrix; // 没有积分型贡献时为空
    };

    ThermalInterfaceAssembly(FemSystem& fem, const ThermalField& field);

    void connect(const ThermalInterfaceContext& context); // 找出理想接触界面两侧对应的自由度，并把它们登记为同一组，后续由同一个温度未知量表示

    void add_transfer(const ThermalInterfaceContext& context, mfem::Coefficient& conductance); // 把当前界面片的传热贡献，累加到界面总矩阵 matrix_ 中

    Operators finalize(); // 完成界面矩阵的组装，并返回约束映射 P 和界面传热矩阵 K_I

private:
    int root(int dof); // 找到某个自由度所属分组的最终代表编号

    std::unique_ptr<mfem::SparseMatrix> make_prolongation(); // 根据已经建立的自由度分组 生成映射矩阵 P 使 T完整 = PT独立

    const ThermalField& field_; // 试探场

    ThermalInterfaceIntegrator integrator_; // 将 q=h(T_a-T_b) 积分为双侧传热矩阵; h 可以在各界面积分点求值

    std::unique_ptr<mfem::SparseMatrix> matrix_; // 保存所有热阻界面片组装得到的界面传热矩阵 K_I，后续加入热问题的总导热矩阵 K=K_{\mathrm{域内}}+K_{\mathrm{Robin}}+K_I.

    std::unique_ptr<mfem::H1_FECollection> collection_; // 拥有一套辅助 H1 有限元集合，与分域场采用相同阶次，用于建立下面的父连续空间

    std::unique_ptr<mfem::FiniteElementSpace> parent_space_; // 拥有建立在原网格上的辅助连续有限元空间 它为共享界面上的同一自由度提供共同编号，用这个编号配对两侧分域自由度，避免比较浮点坐标

    // 保存自由度分组中的“父节点编号” representative_[自由度编号] = 它指向的父自由度编号;
    // 例如 representative_[5]  = 5;
    //      representative_[12] = 5;
    //      representative_[20] = 12;
    // 对应 20 -> 12 -> 5 -> 自己 含义是：自由度 20、12、5 已被连接到同一组，应使用同一个温度未知量 注意: 数组里存的不一定是最终代表编号 例如 representative_[20] 是 12，但 root(20) 沿链查找后得到最终代表 5
    // 当前同心多层颗粒可以有多个 region，但每个材料界面只连接相邻的两个区域，各层界面之间互不相交 因此理想接触的自由度分组通常只有两个成员
    // 未来如果在包覆层中穿孔，孔内又定义为气体等独立区域 多个区域在公共边或顶点相交时，就需要合并三方或更多自由度的连续关系
    std::vector<int> representative_; // 初始时 representative_[i] = i; 每个自由度都指向自己，表示彼此独立 理想接触的 connect() 会合并这些分组
};

}
