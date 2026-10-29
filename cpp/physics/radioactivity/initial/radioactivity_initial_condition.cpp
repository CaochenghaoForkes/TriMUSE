#include "physics/radioactivity/initial/radioactivity_initial_condition.hpp"
#include "fem/geometric_weighted_coefficient.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace trimuse
{

namespace
{

class InitialConcentrationCoefficient final : public mfem::Coefficient
{
public:
    explicit InitialConcentrationCoefficient(const std::unordered_map<int, double>& region_values) : region_values_(region_values) {}

    double Eval(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint&) override
    {
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

std::unordered_map<int, double> initial_values(const FemSystem& fem_system,
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
        region_values[config.region_attribute] = inventory / fem_system.mesh_data().region_volumes().at(config.region_attribute);
    }
    return region_values;
}

}

void initialize_concentration(const FemSystem& fem_system, FieldData& concentration_field, const std::vector<RadioactivityInitialConfig>& initial_configs)
{
    const auto region_values = initial_values(fem_system, initial_configs);
    const double uniform_value = region_values.begin()->second;
    if (std::all_of(region_values.begin(), region_values.end(), [uniform_value](const auto& item) { return item.second == uniform_value; }))
    {
        concentration_field.assign(uniform_value);
        return;
    }

    // 区域阶跃做带几何权重的 L2 投影, 不覆盖公共节点; 投影不保证逐点非负。
    mfem::GridFunction& concentration = concentration_field.mfem_grid_function();
    mfem::FiniteElementSpace* space = concentration.FESpace();
    InitialConcentrationCoefficient initial_concentration(region_values);
    mfem::ConstantCoefficient unit_coefficient(1.0);
    GeometricWeightedCoefficient weighted_unit_coefficient(unit_coefficient, fem_system);
    GeometricWeightedCoefficient weighted_initial_concentration(initial_concentration, fem_system);
    mfem::BilinearForm mass_form(space);
    mass_form.AddDomainIntegrator(new ProjectionMassIntegrator(weighted_unit_coefficient, fem_system));
    mass_form.Assemble();
    mfem::LinearForm right_hand_side(space);
    right_hand_side.AddDomainIntegrator(new ProjectionLFIntegrator(weighted_initial_concentration, fem_system));
    right_hand_side.Assemble();

    mfem::Array<int> essential_true_dofs;
    mfem::SparseMatrix system_matrix;
    mfem::Vector true_concentration, true_right_hand_side;
    mass_form.FormLinearSystem(essential_true_dofs, concentration, right_hand_side, system_matrix, true_concentration, true_right_hand_side);
    mfem::GSSmoother preconditioner(system_matrix);
    mfem::CGSolver solver;
    solver.SetOperator(system_matrix);
    solver.SetPreconditioner(preconditioner);
    solver.SetRelTol(1e-12);
    solver.SetAbsTol(0.0);
    solver.SetMaxIter(1000);
    solver.SetPrintLevel(0);
    solver.Mult(true_right_hand_side, true_concentration);
    if (!solver.GetConverged())
    {
        throw std::runtime_error("初始浓度的 L2 投影未收敛");
    }
    mass_form.RecoverFEMSolution(true_concentration, right_hand_side, concentration);
}

void initialize_concentration(const FemSystem& fem_system, RegionFieldData& concentration_field,
    const std::vector<RadioactivityInitialConfig>& initial_configs)
{
    for (const auto& [region, value] : initial_values(fem_system, initial_configs))
    {
        concentration_field.field_data(region).assign(value);
    }
}

}
