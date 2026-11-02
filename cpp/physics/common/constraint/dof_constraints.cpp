#include "physics/common/constraint/dof_constraints.hpp"

#include <numeric>
#include <unordered_map>

namespace trimuse
{

DofConstraints::DofConstraints(FemSystem& fem, const TrialField& trial_field)
    : trial_field_(trial_field) // 传入试探场；只管理其自由度连接关系，不执行物理积分
{
    if (!trial_field.regional()) { return; } // 连续场已经共享界面自由度，不需要后面的辅助配对空间和分组数组，直接结束构造函数体
    const auto& space = trial_field.space(fem.mesh_data().region_attributes().front()); // 第一个区域编号 → 该区域的场空间
    const int order = space.GetMaxElementOrder(); // 最高单元插值阶次
    collection_ = std::make_unique<mfem::H1_FECollection>(order, fem.mesh_data().dimension()); // 建立同阶次、对应问题维数的 H1 有限元集合
    parent_space_ = std::make_unique<mfem::FiniteElementSpace>(&fem.mesh_data().mfem_mesh(), collection_.get(), space.GetVDim(), space.GetOrdering()); // 在原网格上建立同分量数、同排列方式的辅助连续空间
    representative_.resize(trial_field.size()); // 让分组数组长度等于试探场的完整自由度总数，每个自由度对应一项
    std::iota(representative_.begin(), representative_.end(), 0); // 依次填入 {0, 1, 2, 3, ...}
}

int DofConstraints::root(int dof) // 找到某个自由度所属分组的最终代表编号
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

void DofConstraints::connect(const MeshData::Interface& interface) // 找出理想接触界面两侧对应的自由度，并把它们登记为同一组，后续由同一个场值未知量表示
{
    if (!trial_field_.regional()) { return; } // 如果是连续场，相邻单元已经共享界面自由度，不需要额外连接
    // 当前界面片的信息包括原网格面编号，以及两侧区域、域单元编号。
    mfem::Array<int> dofs[2]; // dofs 包含两个 mfem::Array<int> 的数组, 每个的长度可变; 取得两侧整个域单元的自由度编号 dofs[0] 第一侧单元 采用分域场整体编号 dofs[1] 第二侧单元 采用分域场整体编号 
    for (int side = 0; side < 2; ++side) 
    { 
        trial_field_.element_dofs(interface.sides[side].element, dofs[side]); // interface.sides[side].element side 侧域单元编号(原网格编号)
    }
    // 以父连续空间的自由度编号配对, 不比较坐标或假定两侧面的局部排列相同
    std::unordered_map<int, int> first; // 后续将保存 父连续空间的自由度编号 -> 第一侧分域场的整体自由度编号
    mfem::Array<int> parent_dofs, face_dofs; // 创建两个长度可变的整数数组 parent_dofs(接收某个域单元在父连续空间中的自由度编号) face_dofs(接收当前界面片在父连续空间中的自由度编号，后面才会填入)
    parent_space_->GetElementVDofs(interface.sides[0].element, parent_dofs); // interface.sides[0].element 取得界面第一侧的原网格域单元编号 parent_space_->GetElementVDofs(单元编号, parent_dofs); 在辅助父连续空间中，查找这个单元的全部分量的自由度编号，并自动调整、填充 parent_dofs
    for (int j = 0; j < parent_dofs.Size(); ++j) 
    { 
        first[parent_dofs[j]] = dofs[0][j]; // 按单元内部的对应排列, 建立第一个单元中的自由度在父连续空间编号 -> 第一侧分域场整体编号
    }
    parent_space_->GetFaceVDofs(interface.face, face_dofs); // 取得当前界面上的父空间自由度存入 face_dofs
    parent_space_->GetElementVDofs(interface.sides[1].element, parent_dofs); // 再取得第二侧单元的父空间自由度编号，覆盖前面的 parent_dofs
    for (int j = 0; j < parent_dofs.Size(); ++j)
    {
        if (face_dofs.Find(parent_dofs[j]) != -1) // 判断当前第二侧自由度是否位于界面上 没找到则返回 -1
        {
            representative_[root(dofs[1][j])] = root(first.at(parent_dofs[j])); // first.at(parent_dofs[j]) 取得它在父连续空间中的编号，再用这个共同编号，从 first 表中找到第一侧对应的分域自由度 dofs[1][j] 取得第二侧单元当前的分域自由度编号 分别找到两个自由度所属组的最终代表 让第二组的根指向第一组的根，完成分组合并
        }
    }
}

void DofConstraints::finalize()
{
    prolongation_ = make_prolongation();
    // 固定约束构建完毕，只需保留 P；辅助空间和分组数组不再参与求解。
    parent_space_.reset();
    collection_.reset();
    representative_ = std::vector<int>{};
}

std::unique_ptr<mfem::SparseMatrix> DofConstraints::make_prolongation() // 根据已经建立的自由度分组 生成映射矩阵 P 使 x完整 = Px独立
{
    if (!trial_field_.regional()) // 先处理连续场
    {
        const auto* conforming = trial_field_.space(0).GetConformingProlongation(); // 取得 MFEM 自身的协调约束映射，例如用于处理悬挂节点
        if (conforming) 
        { 
            return std::make_unique<mfem::SparseMatrix>(*conforming); // 如果存在这个映射，就复制一份返回 原矩阵由 MFEM 管理，返回的副本由我们自己的 unique_ptr 管理
        }
        auto identity = std::make_unique<mfem::SparseMatrix>(trial_field_.size()); // 如果没有额外协调约束，完整自由度与独立自由度一致，创建单位矩阵
        for (int i = 0; i < trial_field_.size(); ++i)
        { 
            identity->Add(i, i, 1.0); 
        }
        identity->Finalize(); // 整理稀疏存储格式，并返回
        return identity;
    }

    // 分域场：给每个自由度分组安排独立编号
    std::unordered_map<int, int> columns; // 创建映射表: 分组的最终代表编号 -> 独立未知量编号，也就是 P 的列号
    for (int i = 0; i < trial_field_.size(); ++i) // 遍历全部完整自由度
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
    auto prolongation = std::make_unique<mfem::SparseMatrix>(trial_field_.size(), columns.size()); // 建立“独立向量 -> 完整向量”的矩阵 行数=完整自由度数 列数=独立未知量数
    for (int i = 0; i < trial_field_.size(); ++i)
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
        // 独立场向量为 u = [u0, u1, u2]^T，则：
        //
        // x_full = P * u = [u0, u1, u0, u2, u1]^T
    }
    prolongation->Finalize();
    return prolongation;
}

mfem::Vector DofConstraints::reduced_values() const // 用于把试探场的完整场向量，转换成约束后的独立场向量，供后续求解使用
{
    const auto& p = prolongation(); // 取得映射 P x_full = P * x_independent
    mfem::Vector full = trial_field_.values(), reduced(p.Width()), ones(p.Height()), weights(p.Width()); // full 当前试探场值的完整向量 reduced 要返回的独立场向量 ones 辅助的完整空间全一向量 weights 记录每组包含多少个完整自由度 P.Height()：完整自由度数 P.Width() ：独立自由度数
    if (!trial_field_.regional()) // 如果是连续场
    {
        const auto* restriction = trial_field_.space(0).GetConformingRestriction(); // 取得 MFEM 的限制矩阵 R，用于将完整自由度转换为协调约束下的独立自由度
        if (restriction) 
        { 
            restriction->Mult(full, reduced); // 存在约束时计算 x独立 = Rx完整
            return reduced; 
        }
        return full;
    }
    if (p.Height() == p.Width()) // 分域场没有自由度合并时直接返回 例如所有界面均保留两侧独立自由度时，就属于这种情况
    { 
        return full; 
    }
    ones = 1.0; // 辅助向量全部元素设为1
    p.MultTranspose(full, reduced); // 计算 reduced = P^T full
    p.MultTranspose(ones, weights); // 计算 weights = P^T 1 因此每一项就是对应组的成员数量
    // 原始关系 x_full = Pu
    // P^T x_full = P^TPu
    // u = (P^T P)^{-1} P^T x_full
    // P^T P 是对角阵,对角值就是各组成员数, 所以 reduced 还得除 weights 
    for (int i = 0; i < reduced.Size(); ++i)
    { 
        reduced[i] /= weights[i]; 
    }
    // 例：区域 a、b 各有两个自由度，完整向量按 [a0, a1, b0, b1] 排列。
    // 相等约束要求 x_a1 == x_b0，因此独立向量为 [x_a0, x_interface, x_b1]。
    //
    // P = [ 1  0  0 ]
    //     [ 0  1  0 ]
    //     [ 0  1  0 ]
    //     [ 0  0  1 ]
    //
    // 从独立向量恢复完整向量：
    // u    = [300, 350, 400]^T
    // P*u  = [300, 350, 350, 400]^T
    //
    // 反过来提取独立向量时，P^T 会将同组自由度的场值相加：
    // P^T * full = [300, 700, 400]^T
    //
    // 因为 P^T*P = diag(1, 2, 1)，不能直接把上述结果作为独立场值。
    // 用全一向量计算各组成员数：
    // weights = P^T * ones = [1, 2, 1]^T
    //
    // 再逐项相除，得到组内平均场值：
    // reduced = [300/1, 700/2, 400/1]^T = [300, 350, 400]^T
    //
    // 若完整向量暂为 [300, 340, 360, 400]^T，界面独立场值同样得到：
    // x_interface = (340 + 360) / 2 = 350。
    // 整个过程对应 reduced = (P^T*P)^(-1) * P^T * full
    return reduced;
}

void DofConstraints::form_system(const mfem::SparseMatrix& matrix, const mfem::Vector& rhs, const std::map<int, double>& essential, mfem::SparseMatrix& system, mfem::Vector& load, mfem::Vector& solution) const // 把完整自由度上的方程，转换成独立自由度系统，并施加 Dirichlet 场值约束 输入 Ax=b 输出 system(最终系统矩阵)、load(最终右端)、solution(独立自由度空间中的场值初始猜测); matrix 与 rhs: 已经组合好的完整矩阵与右端 
{
    const auto& p = prolongation(); // 取得约束映射 P x=Pu
    if (p.Height() == p.Width()) // 没有自由度约化时直接复制
    {
        system = matrix;
        load = rhs;
    }
    else // 有约束时转换到独立空间 约化方程：把相互约束的自由度对应的平衡方程合并；约化场值初始值：给同组自由度选出一个代表场值
    {
        // 约束要求 x = Pu
        // 但加入约束后，原来各个自由度的方程不再独立 不能只代入后，仍要求所有完整空间方程分别成立；连接约束可能在两侧产生相互抵消的约束反力或通量
        // 从弱形式看，测试函数也必须满足相同的连续约束。若独立测试系数为 z，完整测试系数就是 w = Pz
        // 带入离散弱形式 w^T(Ax-b) = 0
        // 得到 z^TP^T(APu-b) = 0
        // 因为是对任意的 z 都成立 所以 P^T A P u = P^T b
        std::unique_ptr<mfem::SparseMatrix> reduced(mfem::RAP(p, matrix, p)); // 该方法计算 A独立 = P^T A P
        system = *reduced; // 将约化矩阵保存到输出 system
        load.SetSize(p.Width()); p.MultTranspose(rhs, load); // 右端转换为 b独立 = P^T b 这里右端不除以组成员数，因为约化方程需要将同组自由度的方程贡献相加，与前面构造场值初始猜测时的组内平均不同
    }
    solution = reduced_values(); // 将当前完整试探场值转换为独立自由度向量
    // 指定值由物理模块收集：独立自由度编号 → 指定场值。
    for (const auto& [dof, value] : essential) // 遍历所有 Dirichlet 约束
    {
        solution[dof] = value; // 先让求解初始猜测满足指定场值
        system.EliminateRowCol(dof, value, load); // 将 udof = value 施加到系统中 从其它行中消去该列 将已知场值移动到右端 将对应行改成固定该场值的方程 同时处理行与列，保持矩阵对称性 Dirichlet 消元后，这个自由度仍保留在向量里，只是其场值已经固定
    }
}

mfem::Vector DofConstraints::recover(const mfem::Vector& solution) const // 传入独立自由度场向量 把独立自由度场值恢复成完整场向量，并返回完整向量 
{
    mfem::Vector full(trial_field_.size()); // 创建长度等于完整自由度数的向量
    prolongation().Mult(solution, full); // 执行矩阵乘法 x完整 = P x独立
    return full;
}

}
