#include "physics/thermal/interface/thermal_interface_integrator.hpp"

namespace trimuse
{

void ThermalInterfaceIntegrator::assemble(const ThermalInterfaceContext& context, mfem::Coefficient& conductance, mfem::SparseMatrix& matrix) const // 把当前界面片的有限热阻传热关系，积分成耦合两侧单元的局部矩阵，再累加到整体界面矩阵中 context: 当前界面片及其双侧场与几何信息 conductance: 界面传热系数 h=1/R 的求值对象 matrix: 接收组装结果的整体稀疏矩阵使用完整自由度编号 (整个热界面模块共用的一张矩阵，保存所有热阻界面片的传热贡献 后续会传给热问题最终的系统矩阵)
{
    const auto& interface = context.interface(); // 取得当前界面片信息
    const auto& field = context.field(); // 取得试探场访问对象
    mfem::Array<int> dofs[2]; // 分别取得界面两侧单元中包含的自由度在分域场整体自由度中编号的列表
    for (int side = 0; side < 2; ++side) 
    { 
        field.element_dofs(interface.sides[side].element, dofs[side]); // 根据原网格的域单元编号，将该单元对应的分域场整体自由度编号填入 dofs[side]
    }
    mfem::Array<int> joined(dofs[0].Size() + dofs[1].Size()); // 将两侧的自由度拼起来 如 dofs[0]={10,11,12} dofs[1]={20,21,22} 得 joined={10,11,12,20,21,22}
    for (int i = 0; i < dofs[0].Size(); ++i) 
    { 
        joined[i] = dofs[0][i]; 
    }
    for (int i = 0; i < dofs[1].Size(); ++i) 
    { 
        joined[dofs[0].Size() + i] = dofs[1][i]; 
    }

    const auto& rule = context.quadrature_rule(); // 取得当前参考面上的求积规则，包含积分点坐标和权重
    auto& transformation = context.transformations(); // 取得当前面的双侧几何变换，用来将参考面点映射到两侧参考单元，并计算几何积分权重
    mfem::Vector shape[2], jump(joined.Size()); // shape: 长度为2(代表两侧)的数组,每个元素都是 mfem::Vector jump: 长度等于两侧单元自由度之和的 mfem::Vector
    mfem::DenseMatrix local(joined.Size()); local = 0.0; // 创建一个方阵并清零 规模与两侧单元自由度之和相同
    for (int q = 0; q < rule.GetNPoints(); ++q) // 遍历界面积分点
    {
        const auto& point = rule.IntPoint(q); // 取得参考面上第 q 个积分点 point
        transformation.SetAllIntPoints(&point); // 设置当前积分点
        for (int side = 0; side < 2; ++side) // 准备计算两侧形函数值
        {
            const auto& location = interface.sides[side]; // 取得当前 side 侧的区域编号、原网格中单元编号
            const auto* element = field.space(location.region_attribute).GetFE(field.local_element(location.element)); // 先将原单元编号转换为所属场空间中的局部单元编号，再取得这个单元的有限元对象
            shape[side].SetSize(element->GetDof()); // 按照该有限元的自由度数量，设置形函数值向量的长度
            element->CalcShape(side == 0 ? transformation.GetElement1IntPoint() : transformation.GetElement2IntPoint(), shape[side]); // 在当前界面积分点对应的参考单元位置，计算当前侧所有形函数的值，并写入 shape[side] .GetElement1IntPoint() 返回参考面点 point 映射到第一侧参考单元后得到的坐标 CalcShape 将参考坐标带入该单元所有形函数计算 并存入 shape[side]
        }
        // 构造形函数值列向量 B=[N_a; -N_b], 自由度温度列向量 t=[t_a; t_b]。
        // B^T*t = T_a - T_b, 即同一实际界面点两侧温度的差。
        for (int i = 0; i < shape[0].Size(); ++i) // 第一侧取正
        { 
            jump[i] = shape[0][i]; 
        }
        for (int i = 0; i < shape[1].Size(); ++i) 
        { 
            jump[shape[0].Size() + i] = -shape[1][i]; // 构造形函数跳跃向量 第二侧取负
        }
        // weight = hq * wq * J_gamma,q * gq
        const double weight = conductance.Eval(transformation, point) * point.weight * transformation.Weight() * fem_.geometric_weight(transformation, point); // conductance.Eval(...) 当前积分点的传热系数 h; point.weight 参考面上的求积权重; transformation.Weight 面几何映射的雅可比权重; geometry_weight 1D 4pir2 2D 2pir 3D 1
        // 界面推导沿用 weak_form.md 的 Galerkin 方法, i 对应测试函数/矩阵行, j 对应温度自由度/列。
        // 1. 定义 q_I=h(T_a-T_b), h=1/R, 正方向为 a -> b。
        //    两侧外向热流分别为 q_out,a=q_I, q_out,b=-q_I, 保证界面热量守恒。
        // 2. 两侧导热方程分别乘测试函数并分部积分, 左端的界面项合并为
        //    int_Gamma q_I*v_a + int_Gamma (-q_I)*v_b
        //    = int_Gamma h(T_a-T_b)(v_a-v_b)。
        // 3. 用形函数插值温度: T_a=sum_j N_j^a*t_j^a, T_b=sum_j N_j^b*t_j^b。
        //    测试 a 侧第 i 行, 取 v_a=N_i^a, v_b=0, 得到
        //    sum_j (int_Gamma h*N_i^a*N_j^a)*t_j^a - sum_j (int_Gamma h*N_i^a*N_j^b)*t_j^b。
        //    测试 b 侧第 i 行, 取 v_a=0, v_b=N_i^b, 得到
        //    -sum_j (int_Gamma h*N_i^b*N_j^a)*t_j^a + sum_j (int_Gamma h*N_i^b*N_j^b)*t_j^b。
        // 4. 由此得到四块局部矩阵:
        //    K_aa= int_Gamma h*N_a*N_a^T, K_ab=-int_Gamma h*N_a*N_b^T,
        //    K_ba=-int_Gamma h*N_b*N_a^T, K_bb= int_Gamma h*N_b*N_b^T。
        //    令 B=[N_a; -N_b], 合并为 K_Gamma=int_Gamma h*B*B^T。
        // 5. 转到参考面: N_a=hat_N_a(Loc1(s)), N_b=hat_N_b(Loc2(s))。
        //    求积得到 K_Gamma ~= sum_q h_q*w_q*J_Gamma,q*g_q*B_q*B_q^T。
        //    jump 保存 B_q, weight 保存 h_q*w_q*J_Gamma,q*g_q, 当前调用累加一个积分点的贡献。

        mfem::AddMult_a_VVt(weight, jump, local); // local <- local + weight jump jump^T
    }
    matrix.AddSubMatrix(joined, joined, local, 0); // 将当前界面片的局部矩阵 local，按整体自由度编号累加到界面总矩阵 matrix 中 0 表示零项也参与插入
}

}
