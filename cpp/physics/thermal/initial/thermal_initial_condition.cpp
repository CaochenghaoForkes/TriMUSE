#include "physics/thermal/initial/thermal_initial_condition.hpp"
#include "fem/geometric_weighted_coefficient.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace trimuse
{

namespace
{

class InitialTemperatureCoefficient final : public mfem::Coefficient
{
public:
    explicit InitialTemperatureCoefficient(const std::unordered_map<int, double>& region_values) : region_values_(region_values) {}

    double Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint&) override
    {
        // 同一个区域内，无论在哪个积分点，返回的初温都相同 (不需要积分点新鲜)
        // const mfem::IntegrationPoint& 接收这个参数，但函数内部不用它
        return region_values_.at(transformation.Attribute);
    }

private:
    const std::unordered_map<int, double>& region_values_;
};

class ProjectionMassIntegrator final : public mfem::MassIntegrator
{
public:
    ProjectionMassIntegrator(mfem::Coefficient& coefficient, const FemSystem& fem_system)
        : mfem::MassIntegrator(coefficient), fem_system_(fem_system) {}

    void AssembleElementMatrix(const mfem::FiniteElement& element, mfem::ElementTransformation& transformation, mfem::DenseMatrix& matrix) override
    {
        SetIntRule(fem_system_.domain_quadrature_rule(element.GetGeomType()));
        mfem::MassIntegrator::AssembleElementMatrix(element, transformation, matrix);
    }

private:
    const FemSystem& fem_system_;
};

class ProjectionLFIntegrator final : public mfem::DomainLFIntegrator
{
public:
    ProjectionLFIntegrator(mfem::Coefficient& coefficient, const FemSystem& fem_system)
        : mfem::DomainLFIntegrator(coefficient), fem_system_(fem_system) {}

    using mfem::DomainLFIntegrator::AssembleRHSElementVect;

    void AssembleRHSElementVect(const mfem::FiniteElement& element, mfem::ElementTransformation& transformation, mfem::Vector& vector) override
    {
        SetIntRule(fem_system_.domain_quadrature_rule(element.GetGeomType()));
        mfem::DomainLFIntegrator::AssembleRHSElementVect(element, transformation, vector);
    }

private:
    const FemSystem& fem_system_;
};

}

void initialize_temperature(const FemSystem& fem_system, FieldData& temperature_field, const std::vector<ThermalInitialConfig>& initial_configs)
{
    std::unordered_map<int, double> region_values;
    for (const ThermalInitialConfig& config : initial_configs)
    {
        if (config.model_name != "uniform")
        {
            throw std::invalid_argument("尚未支持的初始温度模型: " + config.model_name);
        }
        region_values[config.region_attribute] = std::get<double>(config.parameters.at("uniform_value"));
    }
    const double uniform_value = region_values.begin()->second;
    if (std::all_of(region_values.begin(), region_values.end(), [uniform_value](const auto& item) { return item.second == uniform_value; }))
    {
        temperature_field.assign(uniform_value);
        return;
    }

    // 分区域阶跃通过 L2 投影得到连续场, 不按区域覆盖公共节点
    // \[T_h(\boldsymbol x)=\sum_j N_j(\boldsymbol x)T_j\]
    // \[\min_{\{T_j\}}\frac12\int_\Omega(T_h-T_0)^2\,\mathrm d\Omega\]
    // \[\sum_j\underbrace{\int_\Omega N_iN_j\,\mathrm d\Omega}_{M_{ij}}T_j=\underbrace{\int_\Omega N_iT_0\,\mathrm d\Omega}_{b_i}\]
    mfem::GridFunction& temperature = temperature_field.mfem_grid_function(); // 取得现有温度场的引用，最后的结果直接写到这里
    mfem::FiniteElementSpace* space = temperature.FESpace(); // 取得它所属的有限元空间，决定使用哪些形函数、有哪些自由度、如何组装
    InitialTemperatureCoefficient initial_temperature(region_values); // 系数对象根据当前单元的区域编号，返回对应初温
    mfem::ConstantCoefficient unit_coefficient(1.0);
    GeometricWeightedCoefficient weighted_unit_coefficient(unit_coefficient, fem_system);
    GeometricWeightedCoefficient weighted_initial_temperature(initial_temperature, fem_system);
    mfem::BilinearForm mass_form(space);
    mass_form.AddDomainIntegrator(new ProjectionMassIntegrator(weighted_unit_coefficient, fem_system));
    mass_form.Assemble(); // 组装\[M_{ij}=\int_\Omega N_iN_j\,\mathrm d\Omega\]
    mfem::LinearForm right_hand_side(space);
    right_hand_side.AddDomainIntegrator(new ProjectionLFIntegrator(weighted_initial_temperature, fem_system));
    right_hand_side.Assemble(); // 组装 \[b_i=\int_\Omega N_iT_0\,\mathrm d\Omega\]

    mfem::Array<int> essential_true_dofs; // 投影没有要固定的边界自由度
    mfem::SparseMatrix system_matrix; // 最终系数矩阵
    mfem::Vector true_temperature, true_right_hand_side; // 真实自由度上的温度未知量 真实自由度上的右端向量
    mass_form.FormLinearSystem(essential_true_dofs, temperature, right_hand_side, system_matrix, true_temperature, true_right_hand_side); // 组装 MT=b
    mfem::GSSmoother preconditioner(system_matrix); // 构造 Gauss–Seidel 类预条件器，辅助迭代求解更快收敛
    mfem::CGSolver solver; //使用共轭梯度法
    solver.SetOperator(system_matrix); // 指定矩阵
    solver.SetPreconditioner(preconditioner); // 指定预条件器
    solver.SetRelTol(1e-12);
    solver.SetAbsTol(0.0);
    solver.SetMaxIter(1000);
    solver.SetPrintLevel(0); // 关闭迭代打印信息
    solver.Mult(true_right_hand_side, true_temperature); // 求解 把解写入 true_temperature
    if (!solver.GetConverged()) // 确认数值求解成功
    {
        throw std::runtime_error("初始温度的 L2 投影未收敛");
    }
    mass_form.RecoverFEMSolution(true_temperature, right_hand_side, temperature); // 把真实自由度上的解恢复成 GridFunction 的表示，写回最开始取得的 temperature
}

}
