#pragma once

#include <map>
#include "physics/common/field/trial_field.hpp"

namespace trimuse
{

// H1 场的相等约束：按分量连接两侧自由度，并在完整空间与独立空间之间转换。
// 分域场沿用各区域同阶、协调网格的约定；连续场使用 MFEM 自身的协调约束。
class DofConstraints
{
public:
    DofConstraints(FemSystem& fem, const TrialField& trial_field);

    void connect(const MeshData::Interface& interface); // 找出当前界面两侧对应的自由度，登记为同一组，后续由同一个场未知量表示

    void finalize(); // 所有 connect 完成后调用一次，构建 P 并释放配对数据；后续只做约化与恢复

    const mfem::SparseMatrix& prolongation() const { return *prolongation_; }

    mfem::Vector reduced_values() const; // 从当前完整试探场提取独立自由度上的初始猜测

    mfem::Vector recover(const mfem::Vector& solution) const; // x完整 = P x独立，返回完整向量

    void form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& rhs, const std::map<int, double>& essential, mfem::SparseMatrix& system, mfem::Vector& load, mfem::Vector& solution) const; // essential 已使用独立自由度编号

private:
    int root(int dof); // 找到某个自由度所属分组的最终代表编号

    std::unique_ptr<mfem::SparseMatrix> make_prolongation(); // 根据已经建立的自由度分组生成映射矩阵 P，使 x完整 = P x独立

    const TrialField& trial_field_; // 试探场
    
    std::unique_ptr<mfem::SparseMatrix> prolongation_; // 最终约束映射，行对应完整自由度，列对应独立自由度

    std::unique_ptr<mfem::H1_FECollection> collection_; // 拥有一套辅助 H1 有限元集合，与分域场采用相同阶次，用于建立下面的父连续空间

    std::unique_ptr<mfem::FiniteElementSpace> parent_space_; // 拥有建立在原网格上的辅助连续有限元空间 它为共享界面上的同一自由度提供共同编号，用这个编号配对两侧分域自由度，避免比较浮点坐标

    // 保存自由度分组中的“父节点编号” representative_[自由度编号] = 它指向的父自由度编号;
    // 例如 representative_[5]  = 5;
    //      representative_[12] = 5;
    //      representative_[20] = 12;
    // 对应 20 -> 12 -> 5 -> 自己 含义是：自由度 20、12、5 已被连接到同一组，应使用同一个场值未知量 注意: 数组里存的不一定是最终代表编号 例如 representative_[20] 是 12，但 root(20) 沿链查找后得到最终代表 5
    // 当前同心多层颗粒可以有多个 region，但每个材料界面只连接相邻的两个区域，各层界面之间互不相交 因此理想接触的自由度分组通常只有两个成员
    // 未来如果在包覆层中穿孔，孔内又定义为气体等独立区域 多个区域在公共边或顶点相交时，就需要合并三方或更多自由度的连续关系
    std::vector<int> representative_; // 初始时 representative_[i] = i; 每个自由度都指向自己，表示彼此独立 理想接触的 connect() 会合并这些分组
};

}
