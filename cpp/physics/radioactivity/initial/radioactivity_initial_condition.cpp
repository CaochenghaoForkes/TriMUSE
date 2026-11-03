#include "physics/radioactivity/initial/radioactivity_initial_condition.hpp"
#include "fem/geometric_weighted_coefficient.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace trimuse
{

namespace
{

class InitialConcentrationCoefficient final : public mfem::Coefficient // 将区域编号 -> 初始浓度表包装为 MFEM 可积分的系数
{
public:
    explicit InitialConcentrationCoefficient(const std::unordered_map<int, double>& region_values) : region_values_(region_values) {}

    double Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint&) override // 区域内初值恒定，无须使用积分点坐标
    {
        return region_values_.at(transformation.Attribute); // 当前原网格单元的区域属性编号用于查表
    }

private:
    const std::unordered_map<int, double>& region_values_; // 区域编号 -> 初始浓度 atom/m³
};

class ProjectionMassIntegrator final : public mfem::MassIntegrator // 使用统一求积规则组装带几何权重的 L2 投影质量矩阵
{
public:
    ProjectionMassIntegrator(mfem::Coefficient& coefficient, const FemSystem& fem_system)
        : mfem::MassIntegrator(coefficient), fem_system_(fem_system) {}

    void AssembleElementMatrix(const mfem::FiniteElement& element, mfem::ElementTransformation& transformation, mfem::DenseMatrix& matrix) override
    {
        SetIntRule(fem_system_.domain_quadrature_rule(element.GetGeomType())); // 与其他域内积分采用相同求积点
        mfem::MassIntegrator::AssembleElementMatrix(element, transformation, matrix);
    }

private:
    const FemSystem& fem_system_;
};

class ProjectionLFIntegrator final : public mfem::DomainLFIntegrator // 使用统一求积规则组装初始浓度投影右端
{
public:
    ProjectionLFIntegrator(mfem::Coefficient& coefficient, const FemSystem& fem_system)
        : mfem::DomainLFIntegrator(coefficient), fem_system_(fem_system) {}

    using mfem::DomainLFIntegrator::AssembleRHSElementVect; // 保留基类同名重载

    void AssembleRHSElementVect(const mfem::FiniteElement& element, mfem::ElementTransformation& transformation, mfem::Vector& vector) override
    {
        SetIntRule(fem_system_.domain_quadrature_rule(element.GetGeomType()));
        mfem::DomainLFIntegrator::AssembleRHSElementVect(element, transformation, vector);
    }

private:
    const FemSystem& fem_system_;
};

std::unordered_map<int, double> initial_values(const FemSystem& fem_system, // 将输入的区域盘存转换为区域均匀浓度
    const std::vector<RadioactivityInitialConfig>& initial_configs)
{
    std::unordered_map<int, double> region_values;
    for (const RadioactivityInitialConfig& config : initial_configs)
    {
        if (config.model_name != "uniform_from_inventory")
        {
            throw std::invalid_argument("尚未支持的初始浓度模型: " + config.model_name);
        }
        const double inventory = std::get<double>(config.parameters.at("inventory"));
        region_values[config.region_attribute] = inventory / fem_system.mesh_data().region_volumes().at(config.region_attribute); // atom / m³
    }
    return region_values;
}

}

void initialize_concentration(const FemSystem& fem_system, FieldData& accepted_concentration_field, const std::vector<RadioactivityInitialConfig>& initial_configs) // 连续场：同一初值直接赋值，区域阶跃则做 L2 投影
{
    const auto region_values = initial_values(fem_system, initial_configs);
    const double uniform_value = region_values.begin()->second; // 用第一项检查各区域初值是否一致
    if (std::all_of(region_values.begin(), region_values.end(), [uniform_value](const auto& item) { return item.second == uniform_value; }))
    {
        accepted_concentration_field.assign(uniform_value);
        return;
    }

    // 区域阶跃做带几何权重的 L2 投影, 不覆盖公共节点; 投影不保证逐点非负。
    // 求 c_h=sum_j N_j c_j，使 int_Omega g(c_h-c_0)^2 dOmega 最小；所得线性系统为 M c=b。
    // M_ij=int_Omega g N_i N_j dOmega，b_i=int_Omega g N_i c_0 dOmega。
    mfem::GridFunction& accepted_concentration_values = accepted_concentration_field.mfem_grid_function(); // 投影结果最终写入该正式场
    mfem::FiniteElementSpace* space = accepted_concentration_values.FESpace();
    InitialConcentrationCoefficient initial_concentration(region_values);
    mfem::ConstantCoefficient unit_coefficient(1.0);
    GeometricWeightedCoefficient weighted_unit_coefficient(unit_coefficient, fem_system); // 质量矩阵系数 g
    GeometricWeightedCoefficient weighted_initial_concentration(initial_concentration, fem_system); // 右端系数 g*c_0
    mfem::BilinearForm mass_form(space);
    mass_form.AddDomainIntegrator(new ProjectionMassIntegrator(weighted_unit_coefficient, fem_system));
    mass_form.Assemble(); // 组装 M
    mfem::LinearForm right_hand_side(space);
    right_hand_side.AddDomainIntegrator(new ProjectionLFIntegrator(weighted_initial_concentration, fem_system));
    right_hand_side.Assemble(); // 组装 b

    mfem::Array<int> essential_true_dofs; // 初值投影不施加后续的 Dirichlet 边界约束
    mfem::SparseMatrix system_matrix;
    mfem::Vector true_concentration, true_right_hand_side;
    mass_form.FormLinearSystem(essential_true_dofs, accepted_concentration_values, right_hand_side, system_matrix, true_concentration, true_right_hand_side); // 形成真实自由度上的 M c=b
    mfem::GSSmoother preconditioner(system_matrix); // 质量矩阵的 Gauss-Seidel 预条件器
    mfem::CGSolver solver;
    solver.SetOperator(system_matrix);
    solver.SetPreconditioner(preconditioner);
    solver.SetRelTol(1e-12);
    solver.SetAbsTol(0.0);
    solver.SetMaxIter(1000);
    solver.SetPrintLevel(0);
    solver.Mult(true_right_hand_side, true_concentration); // 解出投影后的初始自由度值
    if (!solver.GetConverged())
    {
        throw std::runtime_error("初始浓度的 L2 投影未收敛");
    }
    mass_form.RecoverFEMSolution(true_concentration, right_hand_side, accepted_concentration_values); // 恢复并写回正式 GridFunction
}

void initialize_concentration(const FemSystem& fem_system, RegionFieldData& accepted_concentration_field,
    const std::vector<RadioactivityInitialConfig>& initial_configs) // 分域场各区域自由度独立，可直接按区域设置均匀初值
{
    for (const auto& [region, value] : initial_values(fem_system, initial_configs))
    {
        accepted_concentration_field.field_data(region).assign(value);
    }
}

}
