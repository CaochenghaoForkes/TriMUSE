#include "physics/thermal/thermal_problem.hpp"

#include <chrono>
#include <cmath>
#include <stdexcept>

namespace trimuse
{
namespace
{
TrialField checked_temperature(FemSystem& fem, TrialField field)
{
    const int region = fem.mesh_data().region_attributes().front();
    if (field.space(region).GetVDim() != 1) // 检查场的分量数是否为 1，因为温度是标量
    {
        throw std::invalid_argument("温度场必须为标量 H1 场");
    }
    return field;
}
}

ThermalProblem::ThermalProblem(FemSystem& fem, FieldData& temperature, PropertiesSystem& properties,
    SolverSystem& solver, OutputManager& output, const std::string& problem_type,
    const std::vector<ThermalInitialConfig>& initial, const std::vector<ThermalDomainConfig>& sources,
    const std::vector<ThermalBoundaryConfig>& boundaries, const std::vector<ThermalInterfaceConfig>& interfaces)
    : ThermalProblem(fem, TrialField(fem, temperature), properties, solver, output, problem_type, initial, sources, boundaries, interfaces)
{
}
ThermalProblem::ThermalProblem(FemSystem& fem, RegionFieldData& temperature, PropertiesSystem& properties,
    SolverSystem& solver, OutputManager& output, const std::string& problem_type,
    const std::vector<ThermalInitialConfig>& initial, const std::vector<ThermalDomainConfig>& sources,
    const std::vector<ThermalBoundaryConfig>& boundaries, const std::vector<ThermalInterfaceConfig>& interfaces)
    : ThermalProblem(fem, TrialField(fem, temperature), properties, solver, output, problem_type, initial, sources, boundaries, interfaces)
{
}
ThermalProblem::ThermalProblem(FemSystem& fem, TrialField temperature, PropertiesSystem& properties,
    SolverSystem& solver, OutputManager& output, const std::string& problem_type,
    const std::vector<ThermalInitialConfig>& initial, const std::vector<ThermalDomainConfig>& sources,
    const std::vector<ThermalBoundaryConfig>& boundaries, const std::vector<ThermalInterfaceConfig>& interfaces)
    : temperature_(checked_temperature(fem, std::move(temperature))), properties_(properties), solver_(solver), output_(output),
      transient_(problem_type == "transient"), assembly_(fem, temperature_, sources, boundaries, interfaces)
{
    assembly_.mark_coupled(properties_, transient_);
    if (temperature_.regional()) { initialize_temperature(*temperature_.region_target(), initial); }
    else { initialize_temperature(fem, *temperature_.continuous_target(), initial); }
    temperature_.load();
    assembly_.project_initial();
    temperature_.accept();
}

void ThermalProblem::accept_temperature()
{
    temperature_.accept();
    // 收敛后的属性与最终温度同步; 上一时间步快照仍由外部调度统一提交。
    properties_.update_field_dependent_materials("temperature", {{"temperature", temperature_.view()}}, true);
}

void ThermalProblem::solve(double time_step, double theta)
{
    output_.clear_solve_information("temperature");
    const auto started = std::chrono::steady_clock::now();
    const auto before = assembly_.counts();
    temperature_.load();
    if (transient_ && time_step == 0.0)
    {
        assembly_.update_boundary(properties_.field_data());
        assembly_.apply_initial_constraints();
        accept_temperature();
        return;
    }
    iterate(time_step, theta);
    output_.append_solve_information("temperature", "solve_elapsed_seconds",
        std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count());
    for (const auto& [name, count] : assembly_.counts())
    {
        const auto old = before.find(name);
        output_.append_solve_information("temperature", name + "_assemblies",
            count - (old == before.end() ? 0 : old->second));
    }
}

void ThermalProblem::iterate(double time_step, double theta)
{
    const mfem::Vector old_temperature = temperature_.values();
    ThermalAssembly::State old;
    mfem::Vector old_rhs(temperature_.size()); old_rhs = 0.0;
    if (transient_ && theta < 1.0)
    {
        old = assembly_.update(properties_.previous_field_data(), true);
        old.conduction->Mult(old_temperature, old_rhs);
        old_rhs *= -1.0;
        old_rhs += *old.load;
        old_rhs *= 1.0 - theta;
    }
    // 在新时刻施加本质边界, 不修改旧温度或已缓存的矩阵。
    assembly_.update_boundary(properties_.field_data());
    assembly_.apply_initial_constraints();
    const bool nonlinear = (!transient_ || theta > 0.0) && assembly_.coupled(properties_, transient_);
    const auto& picard = solver_.nonlinear_config("temperature");
    for (int iteration = 0; iteration < (nonlinear ? picard.max_iterations : 1); ++iteration)
    {
        mfem::Vector previous = temperature_.values(), rhs(temperature_.size());
        ThermalAssembly::State current;
        if (!transient_ || theta > 0.0)
        {
            properties_.update_field_dependent_materials("temperature", {{"temperature", temperature_.view()}}, true);
            current = assembly_.update(properties_.field_data(), transient_);
        }
        std::unique_ptr<mfem::SparseMatrix> matrix;
        if (!transient_)
        {
            matrix = std::make_unique<mfem::SparseMatrix>(*current.conduction);
            rhs = *current.load;
        }
        else
        {
            // 保留原有 theta 离散: M_theta=(1-theta)M_n+theta M_{n+1}。
            std::unique_ptr<mfem::SparseMatrix> mixed_mass;
            const mfem::SparseMatrix* mass = theta == 0.0 ? old.mass.get() : current.mass.get();
            if (theta > 0.0 && theta < 1.0)
            {
                mixed_mass.reset(mfem::Add(1.0 - theta, *old.mass, theta, *current.mass));
                mass = mixed_mass.get();
            }
            mass->Mult(old_temperature, rhs);
            rhs /= time_step;
            rhs += old_rhs;
            if (theta > 0.0)
            {
                rhs.Add(theta, *current.load);
                matrix.reset(mfem::Add(1.0 / time_step, *mass, theta, *current.conduction));
            }
            else
            {
                matrix = std::make_unique<mfem::SparseMatrix>(*mass);
                *matrix *= 1.0 / time_step;
            }
        }
        mfem::SparseMatrix system;
        mfem::Vector load, solution;
        assembly_.form_system(*matrix, rhs, system, load, solution);
        solver_.solve_linear("temperature", system, load, solution, output_);
        assembly_.recover(solution);
        const auto trial = temperature_.values();
        previous -= trial;
        const double increment = previous.Normlinf();
        if (!std::isfinite(increment)) { throw std::runtime_error("导热迭代产生非有限温度"); }
        if (!nonlinear || increment <= picard.absolute_tolerance + picard.relative_tolerance * trial.Normlinf())
        {
            accept_temperature();
            output_.append_solve_information("temperature", "picard_iterations", iteration + 1);
            output_.append_solve_information("temperature", "final_temperature_increment_inf_norm_K", increment);
            return;
        }
    }
    throw std::runtime_error("导热 Picard 迭代未收敛");
}
}
