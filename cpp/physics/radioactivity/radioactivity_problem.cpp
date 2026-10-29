#include "physics/radioactivity/radioactivity_problem.hpp"

#include <chrono>
#include <cmath>
#include <stdexcept>

namespace trimuse
{

RadioactivityProblem::RadioactivityProblem(FemSystem& fem, FieldData& concentration, PropertiesSystem& properties,
    SolverSystem& solver, OutputManager& output, const std::string& problem_type, double decay_constant,
    const std::vector<RadioactivityInitialConfig>& initial, const std::vector<RadioactivityDomainConfig>& sources,
    const std::vector<RadioactivityBoundaryConfig>& boundaries, const std::vector<RadioactivityInterfaceConfig>& interfaces)
    : RadioactivityProblem(fem, TrialField(fem, concentration), properties, solver, output, problem_type, decay_constant, initial, sources, boundaries, interfaces)
{
}
RadioactivityProblem::RadioactivityProblem(FemSystem& fem, RegionFieldData& concentration, PropertiesSystem& properties,
    SolverSystem& solver, OutputManager& output, const std::string& problem_type, double decay_constant,
    const std::vector<RadioactivityInitialConfig>& initial, const std::vector<RadioactivityDomainConfig>& sources,
    const std::vector<RadioactivityBoundaryConfig>& boundaries, const std::vector<RadioactivityInterfaceConfig>& interfaces)
    : RadioactivityProblem(fem, TrialField(fem, concentration), properties, solver, output, problem_type, decay_constant, initial, sources, boundaries, interfaces)
{
}
RadioactivityProblem::RadioactivityProblem(FemSystem& fem, TrialField concentration, PropertiesSystem& properties,
    SolverSystem& solver, OutputManager& output, const std::string& problem_type, double decay_constant,
    const std::vector<RadioactivityInitialConfig>& initial, const std::vector<RadioactivityDomainConfig>& sources,
    const std::vector<RadioactivityBoundaryConfig>& boundaries, const std::vector<RadioactivityInterfaceConfig>& interfaces)
    : concentration_(std::move(concentration)), properties_(properties), solver_(solver), output_(output),
      transient_(problem_type == "transient"), assembly_(fem, concentration_, decay_constant, sources, boundaries, interfaces)
{
    assembly_.mark_coupled(properties_);
    if (concentration_.regional()) { initialize_concentration(fem, *concentration_.region_target(), initial); }
    else { initialize_concentration(fem, *concentration_.continuous_target(), initial); }
    concentration_.load();
    assembly_.project_initial();
    concentration_.accept();
}

void RadioactivityProblem::accept_concentration()
{
    concentration_.accept();
    // 收敛后的属性与最终浓度同步; 上一时间步快照仍由外部调度统一提交。
    properties_.update_field_dependent_materials("concentration", {{"concentration", concentration_.view()}}, true);
}

void RadioactivityProblem::solve(double time_step, double theta)
{
    output_.clear_solve_information("concentration");
    const auto started = std::chrono::steady_clock::now();
    const auto before = assembly_.counts();
    concentration_.load();
    if (transient_ && time_step == 0.0)
    {
        assembly_.update_boundary(properties_.field_data());
        assembly_.apply_initial_constraints();
        accept_concentration();
        return;
    }
    iterate(time_step, theta);
    output_.append_solve_information("concentration", "solve_elapsed_seconds",
        std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count());
    for (const auto& [name, count] : assembly_.counts())
    {
        const auto old = before.find(name);
        output_.append_solve_information("concentration", name + "_assemblies",
            count - (old == before.end() ? 0 : old->second));
    }
}

void RadioactivityProblem::iterate(double time_step, double theta)
{
    const mfem::Vector old_concentration = concentration_.values();
    RadioactivityAssembly::State old;
    mfem::Vector old_rhs(concentration_.size()); old_rhs = 0.0;
    if (transient_ && theta < 1.0)
    {
        old = assembly_.update(properties_.previous_field_data());
        old.diffusion->Mult(old_concentration, old_rhs);
        old_rhs *= -1.0;
        old_rhs += *old.load;
        old_rhs *= 1.0 - theta;
    }
    // 在新时刻施加本质边界, 不修改旧浓度或已缓存的矩阵。
    assembly_.update_boundary(properties_.field_data());
    assembly_.apply_initial_constraints();
    const bool nonlinear = (!transient_ || theta > 0.0) && assembly_.coupled(properties_);
    const auto& picard = solver_.nonlinear_config("concentration");
    for (int iteration = 0; iteration < (nonlinear ? picard.max_iterations : 1); ++iteration)
    {
        mfem::Vector previous = concentration_.values(), rhs(concentration_.size());
        RadioactivityAssembly::State current;
        if (!transient_ || theta > 0.0)
        {
            properties_.update_field_dependent_materials("concentration", {{"concentration", concentration_.view()}}, true);
            current = assembly_.update(properties_.field_data());
        }
        std::unique_ptr<mfem::SparseMatrix> matrix;
        if (!transient_)
        {
            // (Kdiffusion + KRobin + lambda M)c = F。
            matrix = std::make_unique<mfem::SparseMatrix>(*current.diffusion);
            rhs = *current.load;
        }
        else
        {
            // M 固定；K 已包含 lambda M，theta 离散统一作用于扩散、衰变和边界项。
            const auto& mass = theta == 0.0 ? old.mass : current.mass;
            mass->Mult(old_concentration, rhs);
            rhs /= time_step;
            rhs += old_rhs;
            if (theta > 0.0)
            {
                rhs.Add(theta, *current.load);
                matrix.reset(mfem::Add(1.0 / time_step, *mass, theta, *current.diffusion));
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
        solver_.solve_linear("concentration", system, load, solution, output_);
        assembly_.recover(solution);
        const auto trial = concentration_.values();
        previous -= trial;
        const double increment = previous.Normlinf();
        if (!std::isfinite(increment)) { throw std::runtime_error("核素扩散迭代产生非有限浓度"); }
        if (!nonlinear || increment <= picard.absolute_tolerance + picard.relative_tolerance * trial.Normlinf())
        {
            accept_concentration();
            output_.append_solve_information("concentration", "picard_iterations", iteration + 1);
            output_.append_solve_information("concentration", "final_concentration_increment_inf_norm_atom_per_m3", increment);
            return;
        }
    }
    throw std::runtime_error("核素扩散 Picard 迭代未收敛");
}
}
