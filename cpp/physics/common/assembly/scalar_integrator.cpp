#include "physics/common/assembly/scalar_integrator.hpp"

namespace trimuse
{
namespace
{

class PointCoefficient final : public mfem::Coefficient // 把预先计算好的积分点系数数组，包装成 MFEM 积分器能调用的系数对象，并在取值时乘上降维几何权重
{
public:
    PointCoefficient(const FemSystem& fem, const std::vector<double>& values) : fem_(fem), values_(values)
    {
    }

    double Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point) override
    {
        return values_[fem_.domain_quadrature_offset(transformation.ElementNo) + point.index] * fem_.geometric_weight(transformation, point); // transformation.ElementNo 取得当前积分单元的编号 这里组装使用原网格变换，因此它是原网格域单元编号；偏移加 point.index 得到全域积分点编号
    }

private:
    const FemSystem& fem_;

    const std::vector<double>& values_; // 全域积分点系数数组 如 扩散、容量、源项系数
};

}

std::shared_ptr<mfem::SparseMatrix> ScalarIntegrator::domain_matrix(int region, const std::vector<double>& values, bool diffusion) // 组装指定 region 的域内矩阵贡献：根据 diffusion 生成扩散矩阵或容量矩阵 region : 当前要组装的区域编号 values: 已经计算好的全域积分点系数数组(扩散或容量系数) diffusion: 选择积分类型 (true: 扩散项 false: 容量项)
{
    auto result = std::make_shared<mfem::SparseMatrix>(field_.size()); // 创建方形稀疏矩阵，行列数均为场的完整自由度数
    PointCoefficient coefficient(fem_, values); // 把积分点数组包装成 MFEM 系数对象 积分器求值时，它返回 当前积分点系数 * 降维几何权重 它根据积分器调用 Eval() 时传入的当前单元编号和局部积分点编号确定下标
    std::unique_ptr<mfem::BilinearFormIntegrator> integrator; // 通过公共基类指针保存具体积分器 BilinearForm 双线性形式 通常对应左端矩阵项 指的是分别对试探函数(形函数构成的那个试探场) u 和测试函数 v 都是线性的
    if (diffusion)  // 如果是扩散项则对应 DiffusionIntegrator 积分 int k grad(N_i) · grad(N_j) dOmega
    {
        integrator = std::make_unique<mfem::DiffusionIntegrator>(coefficient); // 组装系数乘形函数梯度内积
    }
    else // 如果是容量项则对应 MassIntegrator 积分 int capacity N_i N_j dOmega
    {
        integrator = std::make_unique<mfem::MassIntegrator>(coefficient); // 组装系数乘形函数值的乘积
    }
    mfem::IsoparametricTransformation transformation; // 准备每个单元重复使用的临时对象 当前单元的几何映射
    mfem::DenseMatrix local; // 准备每个单元重复使用的临时对象 当前单元的局部矩阵
    mfem::Array<int> dofs; // 准备每个单元重复使用的临时对象 当前单元对应的完整自由度编号
    for (int element : fem_.mesh_data().region_elements(region)) // 遍历当前区域的原网格单元
    {
        // 场的有限元与自由度 -> 通过 TrialField 取得
        // 单元几何与积分点属性编号 -> 仍采用原网格
        const auto* fe = field_.space(region).GetFE(field_.local_element(element)); // 取得该单元的有限元对象，用来提供形函数 (连续场直接用原单元编号 分域场先转换为该区域子网格中的局部编号)
        fem_.mesh_data().mfem_mesh().GetElementTransformation(element, &transformation); // 从原网格读取这个单元的几何映射，写入 transformation
        integrator->SetIntRule(fem_.domain_quadrature_rule(fe->GetGeomType())); // 根据单元参考几何类型，选择统一的域内求积规则 这保证了 PointCoefficient 使用的积分点编号，与属性数组的编号一致
        integrator->AssembleElementMatrix(*fe, transformation, local); // 真正计算当前单元矩阵，结果写入 local
        field_.element_dofs(element, dofs); // 取得该单元在场完整向量中的自由度编号；分域场会包含区域偏移
        result->AddSubMatrix(dofs, dofs, local, 0); // 按照这些整体行列号累加： result(dofs[i], dofs[j]) += local(i, j);
    }
    result->Finalize(); // 将结果整理为 CSR 格式，再返回共享指针
    return result;
}

std::shared_ptr<mfem::Vector> ScalarIntegrator::domain_load(int region, const std::vector<double>& values) // 组装指定区域的体积源向量，对应弱形式右端 int_omega_region Q Ni dOmega
{
    auto result = std::make_shared<mfem::Vector>(field_.size()); *result = 0.0; // 创建长度等于场完整自由度数的向量，并将所有分量置零
    PointCoefficient coefficient(fem_, values); // 包装积分点源项系数 积分器调用 Eval() 时，根据原单元编号和积分点编号读取 Q，并乘降维几何权重
    mfem::DomainLFIntegrator integrator(coefficient); // 创建域内线性形式积分器 int_omega_region source Ni dOmega
    mfem::IsoparametricTransformation transformation; // 临时变量 当前单元的几何变换
    mfem::Vector local; // 临时变量 当前单元的局部源项向量
    mfem::Array<int> dofs; // 临时变量 当前单元在完整场中的自由度编号
    for (int element : fem_.mesh_data().region_elements(region)) // 遍历该区域的域单元
    {
        const auto* fe = field_.space(region).GetFE(field_.local_element(element)); // 取得场所使用的有限元对象；分域场会先将原单元编号转换为子网格局部编号
        fem_.mesh_data().mfem_mesh().GetElementTransformation(element, &transformation); // 从原网格取得该单元的几何映射
        integrator.SetIntRule(fem_.domain_quadrature_rule(fe->GetGeomType())); // 设置与属性缓存一致的域内求积规则
        integrator.AssembleRHSElementVect(*fe, transformation, local); // 计算单元右端向量并写入 local; RHS 表示右端项
        field_.element_dofs(element, dofs); // 取得当前单元的完整自由度编号
        result->AddElementVector(dofs, local); // 累加到整体编号对应的位置
    }
    return result;
}

std::shared_ptr<mfem::SparseMatrix> ScalarIntegrator::boundary_matrix(const std::vector<int>& boundaries, double h) // 组装指定外边界分区 zone 的矩阵贡献 当前主要对应 Robin 传递边界的左端项 int_gamma_zone h Ni Nj dGamma
{
    auto result = std::make_shared<mfem::SparseMatrix>(field_.size()); // 创建与场完整自由度数对应的方形稀疏矩阵，只保存当前 zone 的贡献
    if (h != 0.0) // 只有系数非零时，才遍历该 zone 的外边界单元
    for (int b : boundaries)
    {
        auto* t = fem_.mesh_data().mfem_mesh().GetBdrFaceTransformations(b); // 根据原网格边界单元编号 b，取得边界面的几何变换，以及与它相邻的域单元变换
        const int element = t->Elem1No; // 取得内侧相邻域单元的原网格编号 外边界只有这一侧
        const int region = fem_.mesh_data().mfem_mesh().GetAttribute(element); // 查询这个相邻域单元所属的区域属性编号
        const auto* fe = field_.space(region).GetFE(field_.local_element(element)); // 取得该相邻域单元的有限元对象 分域场会先映射到对应子网格的局部单元编号
        const auto* rule = fem_.boundary_quadrature_rule(t->GetGeometryType()); // 根据边界面的参考几何类型，取得求积规则
        mfem::Vector shape(fe->GetDof()); // 创建形函数值向量，长度等于相邻域单元的自由度数
        mfem::DenseMatrix local(fe->GetDof()); // 创建并清零该边界单元贡献的局部方阵
        local = 0.0;
        for (int q = 0; q < rule->GetNPoints(); ++q) // 遍历参考面上的积分点
        {
            const auto& ip = rule->IntPoint(q); // 取得参考面点
            t->SetAllIntPoints(&ip); // 将其映射到相邻参考域单元中的对应位置
            fe->CalcShape(t->GetElement1IntPoint(), shape); // 在该参考单元位置计算域形函数值 t->GetElement1IntPoint() 返回参考面点 ip，映射到第一侧参考域单元后得到的坐标
            mfem::AddMult_a_VVt(h * ip.weight * t->Weight() * fem_.geometric_weight(*t, ip), shape, local); // 执行 local += h wq J gq Nq Nq^T 也就是逐项累加 h wq J gq Ni Nj
        }
        mfem::Array<int> dofs; field_.element_dofs(element, dofs); // 取得相邻域单元在完整场值向量中的自由度编号
        result->AddSubMatrix(dofs, dofs, local, 0); // result(dofs[i], dofs[j]) += local(i, j); 将当前边界贡献累加到对应的整体行列位置
    }
    result->Finalize(); // 将矩阵整理为 CSR 格式并返回
    return result;
}

std::shared_ptr<mfem::Vector> ScalarIntegrator::boundary_load(const std::vector<int>& boundaries, double value) // 组装指定外边界分区 zone 的右端向量贡献 int_gamma_zone value Ni dGamma
{
    auto result = std::make_shared<mfem::Vector>(field_.size()); *result = 0.0; // 向量长度对应整个场的完整自由度数
    if (value != 0.0) // 只有系数非零时，才遍历该 zone 包含的外边界单元
    for (int b : boundaries)
    {
        auto* t = fem_.mesh_data().mfem_mesh().GetBdrFaceTransformations(b); // 取得当前边界面及其内侧域单元的几何变换
        const int element = t->Elem1No; // 相邻域内单元的原网格编号
        const int region = fem_.mesh_data().mfem_mesh().GetAttribute(element); // 所属区域编号
        const auto* fe = field_.space(region).GetFE(field_.local_element(element)); // 取得该单元的有限元对象；分域场会先转换单元编号
        const auto* rule = fem_.boundary_quadrature_rule(t->GetGeometryType()); // 取得当前参考面上的求积规则
        mfem::Vector shape(fe->GetDof()), local(fe->GetDof()); local = 0.0; // 创建两个向量，长度均等于相邻域单元的自由度数 shape 存放当前积分点上的形函数值 local 累积当前边界单元的右端贡献，初始为零
        for (int q = 0; q < rule->GetNPoints(); ++q) // 逐积分点累加贡献
        {
            const auto& ip = rule->IntPoint(q); // 取得参考面上的积分点，并计算它在内侧参考域单元中的对应坐标
            t->SetAllIntPoints(&ip);
            fe->CalcShape(t->GetElement1IntPoint(), shape); // 在该位置计算域单元的所有形函数值
            local.Add(value * ip.weight * t->Weight() * fem_.geometric_weight(*t, ip), shape); // local_i += value * wq * J * gq * Ni
        }
        mfem::Array<int> dofs; field_.element_dofs(element, dofs); // 取得相邻域单元的完整自由度编号
        result->AddElementVector(dofs, local); // (*result)[dofs[i]] += local[i];
    }
    return result;
}

}
