#include "physics/mechanics/assembly/mechanics_integrator.hpp"

#include "physics/mechanics/assembly/strain_matrix.hpp"

namespace trimuse
{

namespace
{

const mfem::DenseMatrix strain_weights({// 张量双点乘对应的对角矩阵 W
                                        {1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
                                        {0.0, 1.0, 0.0, 0.0, 0.0, 0.0},
                                        {0.0, 0.0, 1.0, 0.0, 0.0, 0.0},
                                        {0.0, 0.0, 0.0, 2.0, 0.0, 0.0},
                                        {0.0, 0.0, 0.0, 0.0, 2.0, 0.0},
                                        {0.0, 0.0, 0.0, 0.0, 0.0, 2.0}});

}

MechanicsIntegrator::MechanicsIntegrator(FemSystem& fem, const TrialField& trial_field) // 初始化并缓存所有域内求积点的应变矩阵 B
    :
    fem_(fem),
    trial_field_(trial_field),
    dimension_(fem.mesh_data().dimension()),
    strain_matrices_(make_strain_matrices(fem, trial_field))
{
}

std::shared_ptr<mfem::SparseMatrix> MechanicsIntegrator::domain_matrix(int region, const std::vector<ElasticityTensorValue>& values, const PropertiesFieldData::DomainIntegrationPointValues& weights) // 组装 region 贡献的总体刚度矩阵 int_Omega (B^e)^T W C_{global} B^e
{
    auto result = std::make_shared<mfem::SparseMatrix>(trial_field_.size());
    mfem::DenseMatrix weighted_elasticity(6), weighted_stress, contribution, local;
    mfem::Array<int> dofs;
    for (int element : fem_.mesh_data().region_elements(region)) // 遍历区域内域单元编号
    {
        const auto* fe = trial_field_.space(region).GetFE(trial_field_.local_element(element)); // 取得当前域单元
        const auto* rule = fem_.domain_quadrature_rule(fe->GetGeomType()); // 取得当前单元求积规则
        const auto offset = fem_.domain_quadrature_offset(element); // 取得单元的积分点在全域连续积分点数组中的起始位置
        local.SetSize(fe->GetDof() * dimension_); // fe->GetDof() 取得当前单元的标量自由度数，不包含分量数，也就是该单元的插值节点数
        local = 0.0;
        for (int q = 0; q < rule->GetNPoints(); ++q) // 遍历求积点
        {
            const auto& strain = strain_matrices_[offset + q]; // 读取初始化时缓存的 B
            const auto& elasticity = values[offset + q]; // 取得该点的6分量弹性矩阵
            const mfem::DenseMatrix elasticity_matrix(elasticity.data); // C，按行列复制六分量弹性矩阵
            weighted_stress.SetSize(6, strain.Width());
            contribution.SetSize(strain.Width());
            mfem::Mult(strain_weights, elasticity_matrix, weighted_elasticity); // W C
            mfem::Mult(weighted_elasticity, strain, weighted_stress); // W C B
            mfem::MultAtB(strain, weighted_stress, contribution); // B^T W C B
            const double weight = std::get<double>(weights[offset + q].value());
            local.Add(weight, contribution); // K^e += weight * B^T W C B
        }
        trial_field_.element_dofs(element, dofs); // GetElementVDofs 的局部顺序固定为 [u_1的全部节点，u_2的全部节点，...]
        result->AddSubMatrix(dofs, dofs, local, 0);
    }
    result->Finalize();
    return result;
}

std::shared_ptr<mfem::Vector> MechanicsIntegrator::domain_load(int region, const std::vector<VectorValue>& values, const PropertiesFieldData::DomainIntegrationPointValues& weights) // 组装 region 内体积力贡献的总体载荷 int_Omega (N^e)^T b
{
    auto result = std::make_shared<mfem::Vector>(trial_field_.size()); // 完整自由度长度，只累加当前区域的贡献
    *result = 0.0;
    mfem::Vector shape, local;
    mfem::Array<int> dofs;
    for (int element : fem_.mesh_data().region_elements(region))
    {
        const auto* fe = trial_field_.space(region).GetFE(trial_field_.local_element(element)); // 取得当前有限元对象
        const auto* rule = fem_.domain_quadrature_rule(fe->GetGeomType()); // 取得当前求积规则
        const auto offset = fem_.domain_quadrature_offset(element); // 取得当前单元积分点全局偏移量
        shape.SetSize(fe->GetDof());
        local.SetSize(fe->GetDof() * dimension_);
        local = 0.0;
        for (int q = 0; q < rule->GetNPoints(); ++q) // 遍历积分点
        {
            const auto& point = rule->IntPoint(q); // 取得当前积分点位置
            fe->CalcShape(point, shape); // 计算形函数
            const auto& force = values[offset + q]; // 直接读取体积力，如重力模型已包含 rho*g
            const double weight = std::get<double>(weights[offset + q].value());
            for (int c = 0; c < dimension_; ++c) // 遍历维度
                for (int i = 0; i < shape.Size(); ++i) // 遍历插值节点
                {
                    local[c * shape.Size() + i] += weight * shape[i] * force[c]; // f^e_{c,i} += wq Jq gq Ni b_c
                }
        }
        trial_field_.element_dofs(element, dofs); // 连续场或分域场的完整自由度编号，局部按分量分块排列
        result->AddElementVector(dofs, local);
    }
    return result;
}

std::shared_ptr<mfem::Vector> MechanicsIntegrator::eigenstrain_load(int region, const std::vector<VectorValue>& values, const PropertiesFieldData::DomainIntegrationPointValues& weights) // 组装 region 贡献的总体本征应变构成的载荷向量 int_Omega (B^e)^T W C_{global} \epsilon^*_{global}
{
    auto result = std::make_shared<mfem::Vector>(trial_field_.size()); // 初始化本征载荷向量
    *result = 0.0;
    mfem::Vector weighted_stress(6), contribution, local;
    mfem::Array<int> dofs;
    for (int element : fem_.mesh_data().region_elements(region)) // 遍历该区域的单元编号
    {
        const auto* fe = trial_field_.space(region).GetFE(trial_field_.local_element(element)); // 取得单元
        const auto* rule = fem_.domain_quadrature_rule(fe->GetGeomType()); // 取得积分规则
        const auto offset = fem_.domain_quadrature_offset(element); // 取得全局积分点中的初始偏移
        local.SetSize(fe->GetDof() * dimension_); // 局部载荷贡献长度为 插值节点数*维数
        local = 0.0;
        for (int q = 0; q < rule->GetNPoints(); ++q) // 遍历积分点
        {
            const auto& strain = strain_matrices_[offset + q]; // 读取初始化时缓存的 B
            strain_weights.Mult(values[offset + q].data(), weighted_stress); // W beta，beta 已由 manager 计算
            contribution.SetSize(strain.Width());
            strain.MultTranspose(weighted_stress, contribution); // B^T W beta，beta 已由 manager 计算
            const double weight = std::get<double>(weights[offset + q].value());
            local.Add(weight, contribution);
        }
        trial_field_.element_dofs(element, dofs);
        result->AddElementVector(dofs, local);
    }
    return result;
}

std::shared_ptr<mfem::Vector> MechanicsIntegrator::boundary_load(const std::vector<int>& boundaries, const MechanicsBoundaryValue& value, const PropertiesFieldData::FaceIntegrationPointValues& weights) // 组装各 boundaries(当前外边界分区包含的原网格边界单元编号) 单元由于指定面力(vector)或指定压力(double) 贡献的载荷向量
{
    auto result = std::make_shared<mfem::Vector>(trial_field_.size()); // 创建完整自由度长度的载荷向量，并全部清零
    *result = 0.0;
    const auto* prescribed_traction = std::get_if<VectorValue>(&value); // 若保存的数面力向量，则返回该向量的指针，若保存压力标量，则返回 nullptr
    mfem::Vector shape, traction(dimension_), local; // 当前求积点上各标量形函数的值 当前求积点的面力分量 当前边界单元贡献的局部载荷向量
    mfem::Array<int> dofs; // 局部位移自由度对应的完整自由度编号
    for (int boundary : boundaries) // 逐个处理边界单元
    {
        auto* transformation = fem_.mesh_data().mfem_mesh().GetBdrFaceTransformations(boundary); // 取得几何映射
        const int face = fem_.mesh_data().mfem_mesh().GetBdrElementFaceIndex(boundary); // 原网格面编号
        const int element = transformation->Elem1No; // 取得外边界内侧相邻的原网格域单元编号
        const int region = fem_.mesh_data().mfem_mesh().GetAttribute(element); // 查该域单元所属区域编号
        const auto* fe = trial_field_.space(region).GetFE(trial_field_.local_element(element)); // 取得该单元
        const auto* rule = fem_.boundary_quadrature_rule(transformation->GetGeometryType()); // 取得求积规则
        shape.SetSize(fe->GetDof()); // 设置标量形函数存储空间
        local.SetSize(fe->GetDof() * dimension_); // 为当前边界贡献创建局部载荷向量，长度为相邻域单元的全部位移自由度
        local = 0.0;
        for (int q = 0; q < rule->GetNPoints(); ++q) // 遍历求积点
        {
            const auto& point = rule->IntPoint(q); // 取得该参考面中当前求积点
            transformation->SetAllIntPoints(&point); // 把它设为当前参考面积分点，同时映射到相邻参考域单元中的对应位置，并更新相应变换对象的当前点
            fe->CalcShape(transformation->GetElement1IntPoint(), shape); // 在这个对应的参考域单元坐标处计算域形函数
            if (prescribed_traction)
            {
                for (int c = 0; c < dimension_; ++c)
                {
                    traction[c] = (*prescribed_traction)[c]; // 若用户直接指定了面力，则直接复制其分量
                }
            }
            else // 若用户指定了法向压力,则需要先计算单位外法向
            {
                if (dimension_ == 1) // 一维边界是端点，需要判断它朝向线单元的哪一侧
                {
                    const double orientation = transformation->Elem1->Jacobian()(0, 0); // 取得参考线单元到实际线单元的映射方向，若为正则参考坐标增大方向与实际坐标增大方向一致
                    const double endpoint = transformation->GetElement1IntPoint().x; // 取得当前边界点在参考线单元中的坐标一般为 0 或 1
                    traction[0] = (endpoint < 0.5 ? -1.0 : 1.0) * (orientation > 0.0 ? 1.0 : -1.0); // 先判断参考端点的外向方向，再结合映射方向，得到实际坐标中的单位外法向 n = +-1
                }
                else
                {
                    mfem::CalcOrtho(transformation->Jacobian(), traction); // 得到垂直于边界切向的向量，其方向对应当前外边界的外法向，但长度一般不是 1
                    traction /= traction.Norml2(); // 归一化得到单位外法向 n
                }
                traction *= -std::get<double>(value); // 取出压力标量并计算 t=-pn (负号表示正压力产生指向材料内部的面力)
            }
            const double weight = std::get<double>(weights[face][q].value());
            for (int c = 0; c < dimension_; ++c) // 遍历位移方向
                for (int i = 0; i < shape.Size(); ++i) // 遍历插值节点
                {
                    local[c * shape.Size() + i] += weight * shape[i] * traction[c]; // += wq Jq hq Ni(xq)tc(xq)
                }
        }
        trial_field_.element_dofs(element, dofs);
        result->AddElementVector(dofs, local);
    }
    return result;
}

void MechanicsIntegrator::recover_state(const std::vector<ElasticityTensorValue>& elasticity, const std::vector<VectorValue>& eigenstrain_coefficients, std::vector<VectorValue>& strain_values, std::vector<VectorValue>& stress_values) const // 从当前完整试探位移中提取各单元的位移，在每个求积点计算应变、应力，并直接写入传入的结果数组 传入弹性矩阵 CV 各求积点点 beta=Cv\epsilonV*
{
    const mfem::Vector displacement = trial_field_.values(); // 取得完整位移场向量
    mfem::Array<int> dofs; // 当前单元的各个位移自由度在完整向量中的编号
    mfem::Vector local_displacement; // 按这些编号提取出的单元位移向量 d^e
    for (int element = 0; element < fem_.mesh_data().element_count(); ++element) // 遍历原网格域单元，提取单元位移
    {
        trial_field_.element_dofs(element, dofs); // 取得当前单元的完整自由度编号
        displacement.GetSubVector(dofs, local_displacement); // 按 dofs 从完整位移向量中提取当前单元的位移 如 d^e=[u_{r,1},u_{r,2},u_{r,3},u_{z,1},u_{z,2},u_{z,3}]^T
        for (std::size_t point = fem_.domain_quadrature_offset(element); point < fem_.domain_quadrature_offset(element + 1); ++point) // 遍历当前求积点
        {
            mfem::Vector strain(strain_values[point].data(), 6); // 为输出数组建立 MFEM 向量视图 .data() 容器方法返回内部连续数据的指针，让 MFEM 向量直接使用这6个数值的存储空间，因此给 strain 写值就会直接修改原来的 strain_values[point]
            mfem::Vector stress(stress_values[point].data(), 6); // 为输出数组建立 MFEM 向量视图 .data() 容器方法返回内部连续数据的指针，让 MFEM 向量直接使用这6个数值的存储空间，因此给 stress 写值就会直接修改原来的 stress_values[point]
            strain_matrices_[point].Mult(local_displacement, strain); // 总应变 epsilon_V = B d
            const mfem::DenseMatrix elasticity_matrix(elasticity[point].data); // 将该点的六乘六弹性矩阵包装成 MFEM 稠密矩阵对象 .data 是我们自己定义的那个6*6数组
            elasticity_matrix.Mult(strain, stress); // sigma_V = C_V epsilon_V，本构计算不乘双点乘权重 W
            for (int component = 0; component < 6; ++component) // 逐分量扣除
            {
                stress[component] -= eigenstrain_coefficients[point][component]; // 扣除 beta = C_V epsilon*_V，当前为热膨胀贡献
            }
        }
    }
}

}
