#include "physics/thermal/interface/thermal_interface_assembly.hpp"

#include <numeric>
#include <unordered_map>

namespace trimuse
{

ThermalInterfaceAssembly::ThermalInterfaceAssembly(FemSystem& fem, const ThermalField& field)
    : field_(field), integrator_(fem) // 传入试探场  传入限元系统用于初始化界面传热积分器，供热阻模型后续使用
{
    if (!field.regional()) { return; } // 连续场已经共享界面自由度，不需要后面的辅助配对空间和分组数组，直接结束构造函数体
    const int order = field.space(fem.mesh_data().region_attributes().front()).GetMaxElementOrder(); // 第一个区域编号 → 该区域的场空间 → 最高单元插值阶次
    collection_ = std::make_unique<mfem::H1_FECollection>(order, fem.mesh_data().dimension()); // 建立同阶次、对应问题维数的 H1 有限元集合
    parent_space_ = std::make_unique<mfem::FiniteElementSpace>(&fem.mesh_data().mfem_mesh(), collection_.get()); // 在原网格上建立辅助连续空间
    representative_.resize(field.size()); // 让分组数组长度等于试探场的完整自由度总数，每个自由度对应一项
    std::iota(representative_.begin(), representative_.end(), 0); // 依次填入 {0, 1, 2, 3, ...}
}

int ThermalInterfaceAssembly::root(int dof) // 找到某个自由度所属分组的最终代表编号
{
    // 例如:
    // representative_[20] = 12;
    // representative_[12] = 5;
    // representative_[5]  = 5;
    // 调用 root(20)
    while (representative_[dof] != dof)
    {
        representative_[dof] = representative_[representative_[dof]]; // representative_[20] = 5
        dof = representative_[dof]; // dof = 5
    }
    return dof; // 返回 5 同时修改 representative_[20] 为 5
}

void ThermalInterfaceAssembly::connect(const ThermalInterfaceContext& context) // 找出理想接触界面两侧对应的自由度，并把它们登记为同一组，后续由同一个温度未知量表示
{
    if (!field_.regional()) { return; } // 如果是连续场，相邻单元已经共享界面自由度，不需要额外连接
    const auto& interface = context.interface(); // 取得当前界面片的信息，包括原网格面编号，以及两侧区域、域单元编号
    mfem::Array<int> dofs[2]; // dofs 包含两个 mfem::Array<int> 的数组, 每个的长度可变; 取得两侧整个域单元的自由度编号 dofs[0] 第一侧单元 采样分域场整体编号 dofs[1] 第二侧单元 采样分域场整体编号 
    for (int side = 0; side < 2; ++side) 
    { 
        field_.element_dofs(interface.sides[side].element, dofs[side]); // interface.sides[side].element side 侧域单元编号(原网格编号)
    }
    // 以父连续空间的自由度编号配对, 不比较坐标或假定两侧面的局部排列相同
    std::unordered_map<int, int> first; // 后续将保存 父连续空间的自由度编号 -> 第一侧分域场的整体自由度编号
    mfem::Array<int> parent_dofs, face_dofs; // 创建两个长度可变的整数数组 parent_dofs(接收某个域单元在父连续空间中的自由度编号) face_dofs(接收当前界面片在父连续空间中的自由度编号，后面才会填入)
    parent_space_->GetElementDofs(interface.sides[0].element, parent_dofs); // interface.sides[0].element 取得界面第一侧的原网格域单元编号 parent_space_->GetElementDofs(单元编号, parent_dofs); 在辅助父连续空间中，查找这个单元的全部标量自由度编号，并自动调整、填充 parent_dofs
    for (int j = 0; j < parent_dofs.Size(); ++j) 
    { 
        first[parent_dofs[j]] = dofs[0][j]; // 按单元内部的对应排列, 建立第一个单元中的自由度在父连续空间编号 -> 第一侧分域场整体编号
    }
    parent_space_->GetFaceDofs(interface.face, face_dofs); // 取得当前界面上的父空间自由度存入 face_dofs
    parent_space_->GetElementDofs(interface.sides[1].element, parent_dofs); // 再取得第二侧单元的父空间自由度编号，覆盖前面的 parent_dofs
    for (int j = 0; j < parent_dofs.Size(); ++j)
    {
        if (face_dofs.Find(parent_dofs[j]) != -1) // 判断当前第二侧自由度是否位于界面上 没找到则返回 -1
        {
            representative_[root(dofs[1][j])] = root(first.at(parent_dofs[j])); // first.at(parent_dofs[j]) 取得它在父连续空间中的编号，再用这个共同编号，从 first 表中找到第一侧对应的分域自由度 dofs[1][j] 取得第二侧单元当前的分域自由度编号 分别找到两个自由度所属组的最终代表 让第二组的根指向第一组的根，完成分组合并
        }
    }
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
    return {make_prolongation(), std::move(matrix_)};
}

std::unique_ptr<mfem::SparseMatrix> ThermalInterfaceAssembly::make_prolongation() // 根据已经建立的自由度分组 生成映射矩阵 P 使 T完整 = PT独立
{
    if (!field_.regional()) // 先处理连续场
    {
        const auto* conforming = field_.space(0).GetConformingProlongation(); // 取得 MFEM 自身的协调约束映射，例如用于处理悬挂节点
        if (conforming) 
        { 
            return std::make_unique<mfem::SparseMatrix>(*conforming); // 如果存在这个映射，就复制一份返回 原矩阵由 MFEM 管理，返回的副本由我们自己的 unique_ptr 管理
        }
        auto identity = std::make_unique<mfem::SparseMatrix>(field_.size()); // 如果没有额外协调约束，完整自由度与独立自由度一致，创建单位矩阵
        for (int i = 0; i < field_.size(); ++i) 
        { 
            identity->Add(i, i, 1.0); 
        }
        identity->Finalize(); // 整理稀疏存储格式，并返回
        return identity;
    }

    // 分域场：给每个自由度分组安排独立编号
    std::unordered_map<int, int> columns; // 创建映射表: 分组的最终代表编号 -> 独立未知量编号，也就是 P 的列号
    for (int i = 0; i < field_.size(); ++i) // 遍历全部完整自由度
    {
        const int representative = root(i); // 通过 root(i) 找到它所属组的最终代表
        if (!columns.count(representative)) // 如果这个组还没有独立编号，就给它安排一个新的编号
        { 
            // 例如
            // 完整自由度 i：  0  1  2  3  4
            // root(i)：      0  1  0  3  1
            // 代表 0 → 独立编号 0 
            // 代表 1 → 独立编号 1
            // 代表 3 → 独立编号 2
            columns.emplace(representative, columns.size()); // columns.size() 是插入前已有的分组数量，因此编号从 0 开始连续递增
        }
    } 
    auto prolongation = std::make_unique<mfem::SparseMatrix>(field_.size(), columns.size()); // 建立“完整自由度 -> 独立自由度”的矩阵 行数=完整自由度数 列数=独立未知量数
    for (int i = 0; i < field_.size(); ++i) 
    { 
        prolongation->Add(i, columns.at(root(i)), 1.0); // root(i) 找到所属组 columns.at(...) 找到该组的独立编号 在对应行列写上 1
        // 例如：完整自由度 0、2 属于同一组，1、4 属于同一组，3 单独成组
        // 共有 5 个完整自由度、3 个独立未知量，因此 P 是 5×3 矩阵：
        //
        //         独立编号
        //          0  1  2
        // P = [    1  0  0    ]  // 完整自由度 0
        //     [    0  1  0    ]  // 完整自由度 1
        //     [    1  0  0    ]  // 完整自由度 2
        //     [    0  0  1    ]  // 完整自由度 3
        //     [    0  1  0    ]  // 完整自由度 4
        //
        // 独立温度向量为 u = [u0, u1, u2]^T，则：
        //
        // T_full = P * u = [u0, u1, u0, u2, u1]^T
    }
    prolongation->Finalize();
    return prolongation;
}

}
