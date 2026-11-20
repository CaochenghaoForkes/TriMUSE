#include "physics/common/assembly/scalar_integrator.hpp"

namespace trimuse
{

std::shared_ptr<mfem::SparseMatrix> ScalarIntegrator::domain_matrix(int region, const std::vector<double>& values, MatrixForm form, const PropertiesFieldData::DomainIntegrationPointValues& weights) // 按缓存权重组装质量型或梯度型区域矩阵
{
    auto result = std::make_shared<mfem::SparseMatrix>(trial_field_.size());
    mfem::IsoparametricTransformation transformation;
    mfem::DenseMatrix gradient, local;
    mfem::Vector shape;
    mfem::Array<int> dofs;
    for (int element : fem_.mesh_data().region_elements(region))
    {
        const auto* fe = trial_field_.space(region).GetFE(trial_field_.local_element(element));
        const int count = fe->GetDof();
        const auto* rule = fem_.domain_quadrature_rule(fe->GetGeomType());
        const auto offset = fem_.domain_quadrature_offset(element);
        local.SetSize(count);
        local = 0.0;
        if (form == MatrixForm::Gradient)
        {
            fem_.mesh_data().mfem_mesh().GetElementTransformation(element, &transformation);
            gradient.SetSize(count, fem_.mesh_data().mfem_mesh().SpaceDimension());
        }
        else
        {
            shape.SetSize(count);
        }
        for (int q = 0; q < rule->GetNPoints(); ++q)
        {
            const auto& point = rule->IntPoint(q);
            const double weight = values[offset + q] * std::get<double>(weights[offset + q].value());
            if (form == MatrixForm::Gradient)
            {
                transformation.SetIntPoint(&point);
                fe->CalcPhysDShape(transformation, gradient); // 第 i 行为形函数 Ni 的物理梯度
                mfem::AddMult_a_AAt(weight, gradient, local); // Kij += weight ∇Ni·∇Nj
            }
            else
            {
                fe->CalcShape(point, shape);
                mfem::AddMult_a_VVt(weight, shape, local); // Mij += weight Ni Nj
            }
        }
        trial_field_.element_dofs(element, dofs);
        result->AddSubMatrix(dofs, dofs, local, 0);
    }
    result->Finalize();
    return result;
}

std::shared_ptr<mfem::Vector> ScalarIntegrator::domain_load(int region, const std::vector<double>& values, const PropertiesFieldData::DomainIntegrationPointValues& weights) // 按缓存权重组装区域源载荷
{
    auto result = std::make_shared<mfem::Vector>(trial_field_.size());
    *result = 0.0;
    mfem::Vector shape, local;
    mfem::Array<int> dofs;
    for (int element : fem_.mesh_data().region_elements(region))
    {
        const auto* fe = trial_field_.space(region).GetFE(trial_field_.local_element(element));
        const auto* rule = fem_.domain_quadrature_rule(fe->GetGeomType());
        const auto offset = fem_.domain_quadrature_offset(element);
        shape.SetSize(fe->GetDof());
        local.SetSize(fe->GetDof());
        local = 0.0;
        for (int q = 0; q < rule->GetNPoints(); ++q)
        {
            fe->CalcShape(rule->IntPoint(q), shape);
            const double weight = values[offset + q] * std::get<double>(weights[offset + q].value());
            local.Add(weight, shape); // Fi += weight Ni
        }
        trial_field_.element_dofs(element, dofs);
        result->AddElementVector(dofs, local);
    }
    return result;
}

std::shared_ptr<mfem::SparseMatrix> ScalarIntegrator::boundary_matrix(const std::vector<int>& boundaries, double h, const PropertiesFieldData::FaceIntegrationPointValues& weights) // 整体矩阵组装（边界积分）：组装指定外边界分区 zone 的矩阵贡献 当前主要对应 Robin 传递边界的左端项 int_gamma_zone h Ni Nj dGamma boundaries 传入的是该外边界分区所包含的原网格边界单元编号列表
{
    auto result = std::make_shared<mfem::SparseMatrix>(trial_field_.size()); // 创建与场完整自由度数对应的方形稀疏矩阵，只保存当前 zone 的贡献
    if (h != 0.0) // 只有系数非零时，才遍历该 zone 的外边界单元
        for (int b : boundaries) // 遍历该外边界分区所包含的原网格边界单元编号列表
        {
            auto* t = fem_.mesh_data().mfem_mesh().GetBdrFaceTransformations(b); // 根据原网格边界单元编号 b，取得边界面的几何变换，以及与它相邻的域单元变换
            const int face = fem_.mesh_data().mfem_mesh().GetBdrElementFaceIndex(b); // 原网格面编号，与面权重缓存一致
            const int element = t->Elem1No; // 取得内侧相邻域单元的原网格编号 外边界只有这一侧
            const int region = fem_.mesh_data().mfem_mesh().GetAttribute(element); // 原网格上查询这个相邻域单元所属的区域属性编号
            const auto* fe = trial_field_.space(region).GetFE(trial_field_.local_element(element)); // 取得该相邻域单元的有限元对象 分域场会先映射到对应子网格的局部单元编号
            const auto* rule = fem_.boundary_quadrature_rule(t->GetGeometryType()); // 根据边界面的参考几何类型，取得求积规则
            mfem::Vector shape(fe->GetDof()); // 创建形函数值向量，长度等于该边界单元相邻域单元的自由度数
            mfem::DenseMatrix local(fe->GetDof()); // 按相邻域单元的全部局部自由度建立方阵；不在该边界面上的自由度，其形函数在面上为零，对应矩阵项也为零
            local = 0.0;
            for (int q = 0; q < rule->GetNPoints(); ++q) // 遍历参考面上的积分点
            {
                const auto& ip = rule->IntPoint(q); // 取得参考面点
                t->SetAllIntPoints(&ip); // 1.把 ip 设为当前边界面的积分点； 2.用 Loc1 找到同一物理位置在相邻域单元参考坐标中的点，并把它设为 Elem1 的当前积分点； 3.若是内部界面且有第二侧，也同样设置 Elem2 的参考积分点
                fe->CalcShape(t->GetElement1IntPoint(), shape); // 在该参考单元位置计算域形函数值 t->GetElement1IntPoint() 返回参考面点 ip 映射到第一侧参考域单元后得到的坐标
                mfem::AddMult_a_VVt(h * std::get<double>(weights[face][q].value()), shape, local); // 执行 local += h wq J gq Nq Nq^T 也就是累加该积分点贡献该边界单元局部矩阵的 h wq J gq Ni Nj
            }
            mfem::Array<int> dofs;
            trial_field_.element_dofs(element, dofs); // 取得相邻域单元在完整场值向量中的自由度编号
            result->AddSubMatrix(dofs, dofs, local, 0); // result(dofs[i], dofs[j]) += local(i, j); 将当前边界单元的局部矩阵贡献累加到对应的整体行列位置
        }
    result->Finalize(); // 将矩阵整理为 CSR 格式并返回
    return result;
}

std::shared_ptr<mfem::Vector> ScalarIntegrator::boundary_load(const std::vector<int>& boundaries, double value, const PropertiesFieldData::FaceIntegrationPointValues& weights) // 整体向量组装（边界积分）：组装指定外边界分区 zone 的右端向量贡献 int_gamma_zone value Ni dGamma
{
    auto result = std::make_shared<mfem::Vector>(trial_field_.size());
    *result = 0.0; // 向量长度对应整个场的完整自由度数
    if (value != 0.0) // 只有系数非零时，才遍历该 zone 包含的外边界单元
        for (int b : boundaries) // 遍历该外边界分区所包含的原网格边界单元编号列表
        {
            auto* t = fem_.mesh_data().mfem_mesh().GetBdrFaceTransformations(b); // 取得当前边界面及其内侧域单元的几何变换
            const int face = fem_.mesh_data().mfem_mesh().GetBdrElementFaceIndex(b); // 原网格面编号，与面权重缓存一致
            const int element = t->Elem1No; // 相邻域内单元的原网格编号
            const int region = fem_.mesh_data().mfem_mesh().GetAttribute(element); // 所属区域编号
            const auto* fe = trial_field_.space(region).GetFE(trial_field_.local_element(element)); // 取得该单元的有限元对象；分域场会先转换单元编号
            const auto* rule = fem_.boundary_quadrature_rule(t->GetGeometryType()); // 取得当前参考面上的求积规则
            mfem::Vector shape(fe->GetDof()), local(fe->GetDof());
            local = 0.0; // 创建两个向量，长度均等于相邻域单元的自由度数 shape 存放当前积分点上的形函数值 local 累积当前边界单元的局部右端，初始为零
            for (int q = 0; q < rule->GetNPoints(); ++q) // 逐积分点累加贡献
            {
                const auto& ip = rule->IntPoint(q); // 取得参考面上的积分点，并计算它在内侧参考域单元中的对应坐标
                t->SetAllIntPoints(&ip); // 1.把 ip 设为当前边界面的积分点； 2.用 Loc1 找到同一物理位置在相邻域单元参考坐标中的点，并把它设为 Elem1 的当前积分点； 3.若是内部界面且有第二侧，也同样设置 Elem2 的参考积分点
                fe->CalcShape(t->GetElement1IntPoint(), shape); // 在该参考单元位置计算域形函数值 t->GetElement1IntPoint() 返回参考面点 ip 映射到第一侧参考域单元后得到的坐标
                local.Add(value * std::get<double>(weights[face][q].value()), shape); // local_i += value * wq * J * gq * Ni
            }
            mfem::Array<int> dofs;
            trial_field_.element_dofs(element, dofs); // 取得相邻域单元的完整自由度编号
            result->AddElementVector(dofs, local); // (*result)[dofs[i]] += local[i]; 将当前边界单元的局部向量贡献累加到对应的整体行列位置
        }
    return result;
}

ScalarIntegrator::ScalarIntegrator(FemSystem& fem, const TrialField& trial_field)
    :
    fem_(fem),
    trial_field_(trial_field)
{
}

}
