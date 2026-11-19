#include "physics/mechanics/constraint/rigid_body_constraints.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <vector>

namespace trimuse
{

namespace
{

struct NodalDof // value{component,position} component:当前位移分量 position:当前节点坐标
{
    int component;
    std::array<double, 3> position;
};

using RigidMode = std::array<double, 6>; // 刚体的6种位移模式（2D实际上只剩一种沿z轴的移动了，但是我们依然保存为6分量形式）

std::map<int, NodalDof> independent_nodal_dofs(const FemSystem& fem, const TrialField& trial_field, const mfem::SparseMatrix& prolongation) // 收集独立自由度 -> {哪个方向分量,哪个位置} 如 result[12] = {1, {0.2, 0.3, 0.0}}; 表示 独立自由度 12，代表坐标 (0.2,0.3) 处节点的 u_z 分量
{
    const auto& mesh = fem.mesh_data().mfem_mesh();
    const int dimension = mesh.Dimension();
    std::map<int, NodalDof> result;
    mfem::Array<int> dofs;
    mfem::Vector position(dimension);
    mfem::IsoparametricTransformation transformation;
    for (int element = 0; element < mesh.GetNE(); ++element) // 遍历域内单元编号
    {
        const int region = mesh.GetAttribute(element);
        const auto* fe = trial_field.space(region).GetFE(trial_field.local_element(element)); // 取得有限元对象
        const auto& nodes = fe->GetNodes(); // 取得当前有限元在参考单元上的全部插值节点位置
        mesh.GetElementTransformation(element, &transformation); // 取得几何映射
        trial_field.element_dofs(element, dofs); // 当前单元中的全部自由度
        for (int node = 0; node < nodes.GetNPoints(); ++node) // 遍历插值节点
        {
            transformation.Transform(nodes.IntPoint(node), position); // 取得当前插值节点的位置
            for (int component = 0; component < dimension; ++component) // 遍历方向分量 建立独立自由度编号 → {位移分量, 节点实际坐标}
            {
                const int full_dof = dofs[component * fe->GetDof() + node]; // 找到“当前节点、当前分量”在数组中的位置，再从 dofs 取出其完整自由度编号
                const int begin = prolongation.GetI()[full_dof]; // 完整自由度对应稀疏约束矩阵 P 的 full_dof 行对应的起始偏移
                NodalDof value{component, {}}; //创建记录 component:当前位移分量 position:当前节点坐标
                for (int c = 0; c < dimension; ++c)
                {
                    value.position[c] = position[c]; // 插入其位置分量
                }
                result.emplace(prolongation.GetJ()[begin], value); // prolongation.GetJ()[begin] 取得对应的独立自由度编号 当前 P 每行只有一个 1，其列号就是独立自由度编号
            }
        }
    }
    return result;
}

RigidMode rigid_mode(const NodalDof& dof, int dimension, const std::array<double, 3>& center, double length) // 传入的位移自由度 dof 对各刚体运动参数的系数 返回该节点该位移分量对六个刚体运动参数的系数
{
    // 三维刚体运动由 3 个平移参数和 3 个转动参数组成
    // a = [ax, ay, az]^T：整体平移
    // omega = [omega_x, omega_y, omega_z]^T：微小转动
    // (x, y, z)：当前节点相对于参考中心的坐标
    //
    // u = a + omega × [x, y, z]^T
    //
    // [ ux ]   [ 1  0  0    0   z  -y ] [   ax    ]
    // [ uy ] = [ 0  1  0   -z   0   x ] [   ay    ]
    // [ uz ]   [ 0  0  1    y  -x   0 ] [   az    ]
    //                                   [ omega_x ]
    //                                   [ omega_y ]
    //                                   [ omega_z ]
    //
    // 展开为：
    // ux = ax + z * omega_y - y * omega_z
    // uy = ay - z * omega_x + x * omega_z
    // uz = az + y * omega_x - x * omega_y
    //
    // 每一行表示当前节点某个位移分量对六个刚体运动参数的系数
    // 代码中坐标已除以 length (整个网格包围盒的最大边长，该放缩使矩阵中的坐标系数 xyz 与平移列的 1.0 数量级接近)，因此对应的转动参数为 length * omega
    // 该缩放不改变约束行之间的线性独立关系

    // 二维采用轴对称模型，只要求 ur、uz，它允许的整体刚体平移只有沿轴线的平移，设平移量为 az
    // az 按 [ar, az, 0, 0, 0, 0] 排列,则
    // [ ur ]   [ 1  0  0  0  0  0 ] [   ar    ]
    // [ uz ] = [ 0  1  0  0  0  0 ] [   az    ]
    //                               [   0     ]
    //                               [   0     ]
    //                               [   0     ]
    //                               [   0     ]
    if (dimension == 2)
    {
        return {0.0, dof.component == 1 ? 1.0 : 0.0, 0.0, 0.0, 0.0, 0.0};
    }
    // 以网格尺度归一化转动列，避免微米级坐标影响平移和转动的秩判断
    const double x = (dof.position[0] - center[0]) / length;
    const double y = (dof.position[1] - center[1]) / length;
    const double z = (dof.position[2] - center[2]) / length;
    // u = a + omega × (x-c)，列顺序为 [ax, ay, az, omega_x, omega_y, omega_z]
    const std::array<RigidMode, 3> rows{{{1.0, 0.0, 0.0, 0.0, z, -y},
                                         {0.0, 1.0, 0.0, -z, 0.0, x},
                                         {0.0, 0.0, 1.0, y, -x, 0.0}}};
    return rows[dof.component];
}

bool add_independent_mode(RigidMode row, std::vector<RigidMode>& basis) // 判断当前这条位移约束，能不能再消除一种尚未被限制的刚体运动 能则把它加入 basis 返回 true 不能则返回 false 不再重复约束
{
    // 对约束行执行 Gram-Schmidt 正交化
    // βₖ = αₖ − Σⱼ₌₁ᵏ⁻¹ [(αₖ · βⱼ) / (βⱼ · βⱼ)] βⱼ
    //
    // αₖ：当前传入的原始向量
    // βⱼ：此前得到的正交向量
    // βₖ：去掉已有方向上的投影后，剩余的正交分量
    //
    // 本质上是判断新向量是否属于已有向量张成的空间，如果属于的话就无事发生，如果不属于，就把它正交化、归一化后加入到基函数空间
    for (int pass = 0; pass < 2; ++pass)
        for (const auto& direction : basis) // direction 是单位向量
        {
            const double projection = std::inner_product(row.begin(), row.end(), direction.begin(), 0.0); // 执行点积 p = r.q （将 r 在 q 上做投影得到 p）
            for (int i = 0; i < 6; ++i)
            {
                row[i] -= projection * direction[i]; // r <- r - (r.q)*q 已有约束没有覆盖的新方向
            }
        }
    const double norm = std::sqrt(std::inner_product(row.begin(), row.end(), row.begin(), 0.0)); // 计算剩余向量长度
    if (norm <= 1e-10)
    {
        return false; // 如果几乎为零，说明当前行可以由已有基线性组合，增加它不能继续消除刚体模态
    }
    for (double& value : row)
    {
        value /= norm; // 把剩余向量归一化
    }
    basis.push_back(row); // 加入正交基
    return true;
}
}

void add_rigid_body_constraints(const FemSystem& fem, const TrialField& trial_field, const DofConstraints& constraints, std::map<int, double>& essential) // 读取已有位移约束，再补上对称条件和必要的定位约束，最终更新 essential 表
{
    const auto& mesh = fem.mesh_data().mfem_mesh(); // 读取网格
    const int dimension = mesh.Dimension(); // 维度
    const auto dofs = independent_nodal_dofs(fem, trial_field, constraints.prolongation()); // 独立自由度 -> {哪个方向分量,哪个位置}
    std::array<double, 3> minimum{}, maximum{}, center{}; // 计算包围盒、参考中心和归一化长度
    for (int c = 0; c < dimension; ++c)
    {
        minimum[c] = maximum[c] = mesh.GetVertex(0)[c]; // 用第一个网格顶点初始化
    }
    for (int vertex = 1; vertex < mesh.GetNV(); ++vertex) // 再遍历其余顶点更新
        for (int c = 0; c < dimension; ++c)
        {
            minimum[c] = std::min(minimum[c], mesh.GetVertex(vertex)[c]);
            maximum[c] = std::max(maximum[c], mesh.GetVertex(vertex)[c]);
        }
    double length = 0.0; // 初始化包围盒长度
    for (int c = 0; c < dimension; ++c)
    {
        center[c] = minimum[c] + 0.5 * (maximum[c] - minimum[c]); // 参考中心（不要求恰好是颗粒的几何中心）
        length = std::max(length, maximum[c] - minimum[c]); // 包围盒长度（用于坐标归一化到 1.0 数量级）
    }

    // 判断 dof 是否位于中心或轴线 1D 对应球心 2D 对应旋转轴
    const auto on_axis = [&](const NodalDof& dof)
    {
        return std::abs(dof.position[0] - minimum[0]) <= 1e-10 * length; // 1e-10 * length 是随网格尺度变化的坐标容差
    };

    if (dimension < 3) // 对于 1D/2D 添加中心或轴线上的径向条件
    {
        for (const auto& [index, dof] : dofs)
        {
            if (dof.component == 0 && on_axis(dof)) // 如果当前自由度是径向位移且位于1D中心或2D轴线
            {
                essential.try_emplace(index, 0.0); // 若本质自由度列表中没它，就把它添加进去并设置值为 0.0 对应 ur=0 (1D球心) ur=0 (2D对称轴)
            }
        }
    }
    if (dimension == 1)
    {
        return; // 对于 1D 球对称位移空间没有整体平移或转动，仅需中心正则性条件，至此已完成固定
    }

    const std::size_t mode_count = dimension == 2 ? 1 : 6; // 判断当前维度需要消除几个独立的刚体模态
    std::vector<RigidMode> basis;
    for (const auto& [index, value] : essential) // 把已有约束对应的系数行依次送去做 GS 正交化
    {
        add_independent_mode(rigid_mode(dofs.at(index), dimension, center, length), basis);
        if (basis.size() == mode_count) // basis.size() 表示这些约束行的秩 即它们已经限制了多少个独立刚体运动参数 若已有条件已经足够，就直接结束
        {
            return;
        }
    }
    for (const auto& [index, dof] : dofs) // 逐个尝试候选自由度
    {
        if (dimension == 2 && !on_axis(dof)) // 二维额外限定候选节点在轴线上，因此轴向定位会选在轴线上
        {
            continue;
        }
        if (add_independent_mode(rigid_mode(dof, dimension, center, length), basis)) // 若它的系数行能被已有基表示，就跳过 若它提供新的独立方向，就把该自由度指定为零
        {
            essential.try_emplace(index, 0.0);
        }
        if (basis.size() == mode_count) // 秩达到目标值后停止
        {
            break;
        }
    }
}

}
