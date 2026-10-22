#include "physics/radioactivity/radioactivity_problem.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace trimuse
{

RadioactivityProblem::RadioactivityProblem(FemSystem& fem_system, FieldData& concentration_field, PropertiesSystem& properties_system,
                                           SolverSystem& solver_system, OutputManager& output_manager,
                                           const std::string& problem_type, double decay_constant,
                                           const std::vector<RadioactivityInitialConfig>& initial_configs,
                                           const std::vector<RadioactivitySourceConfig>& source_configs,
                                           const std::vector<RadioactivityBoundaryConfig>& boundary_configs)
    : fem_system_(fem_system),
      concentration_field_(concentration_field),
      properties_system_(properties_system),
      solver_system_(solver_system),
      output_manager_(output_manager),
      problem_type_(problem_type),
      decay_constant_(decay_constant),
      source_manager_(fem_system, source_configs),
      boundary_manager_(fem_system, boundary_configs),
      diffusion_manager_(fem_system),
      capacity_manager_(fem_system),
      capacity_form_(concentration_field.mfem_grid_function().FESpace())
{
    properties_system_.mark_coupled("concentration", {"radioactivity.nuclide_diffusion_coef"});
    trimuse::initialize_concentration(fem_system_, concentration_field_, initial_configs);
    capacity_manager_.add_integrators(capacity_form_);
    capacity_form_.Assemble();
    capacity_form_.Finalize();
}

void RadioactivityProblem::solve(double time_step, double theta)
{
    output_manager_.clear_solve_information("concentration");
    const auto started = std::chrono::steady_clock::now();
    if (problem_type_ == "steady")
    {
        solve_steady();
    }
    else
    {
        solve_transient(time_step, theta);
    }
    if (problem_type_ == "steady" || time_step != 0.0)
    {
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        output_manager_.append_solve_information("concentration", "solve_elapsed_seconds", elapsed);
    }
}

void RadioactivityProblem::solve_steady()
{
    mfem::GridFunction& concentration = concentration_field_.mfem_grid_function();
    mfem::FiniteElementSpace* space = concentration.FESpace();
    mfem::GridFunction trial_concentration(space);
    trial_concentration = concentration;
    const auto& regions = fem_system_.mesh_data().region_attributes();
    const bool nonlinear = std::any_of(regions.begin(), regions.end(), [this](int region)
    {
        return properties_system_.coupled_with_field("radioactivity.nuclide_diffusion_coef", region, "concentration");
    });
    const NonlinearSolverConfig& picard = solver_system_.nonlinear_config("concentration");
    const int max_iterations = nonlinear ? picard.max_iterations : 1;
    source_manager_.update(properties_system_.field_data());
    boundary_manager_.update(properties_system_.field_data());
    mfem::Array<int> essential_true_dofs;
    boundary_manager_.apply_essential_conditions(trial_concentration, essential_true_dofs);
    mfem::LinearForm source_form(space);
    source_manager_.add_integrators(source_form);
    boundary_manager_.add_integrators(source_form);
    source_form.Assemble();

    for (int iteration = 0; iteration < max_iterations; ++iteration)
    {
        mfem::Vector previous_concentration(trial_concentration);
        properties_system_.update_field_dependent_materials("concentration", {{"concentration", &trial_concentration}}, true);
        diffusion_manager_.update(properties_system_.field_data());
        mfem::BilinearForm diffusion_form(space);
        diffusion_manager_.add_integrators(diffusion_form);
        boundary_manager_.add_integrators(diffusion_form);
        diffusion_form.Assemble();
        diffusion_form.Finalize();

        // (K + lambda M)c = F; 当前为共形串行 H1 空间, 总体与真实自由度一致。
        std::unique_ptr<mfem::SparseMatrix> system_matrix(mfem::Add(1.0, diffusion_form.SpMat(), decay_constant_, capacity_form_.SpMat()));
        mfem::Vector right_hand_side(source_form);
        for (const int dof : essential_true_dofs)
        {
            system_matrix->EliminateRowCol(dof, trial_concentration[dof], right_hand_side);
        }
        solver_system_.solve_linear("concentration", *system_matrix, right_hand_side, trial_concentration, output_manager_);

        previous_concentration -= trial_concentration;
        const double increment = previous_concentration.Normlinf();
        if (!std::isfinite(increment))
        {
            throw std::runtime_error("稳态核素扩散迭代产生非有限浓度");
        }
        if (!nonlinear || increment <= picard.absolute_tolerance + picard.relative_tolerance * trial_concentration.Normlinf())
        {
            concentration = trial_concentration;
            output_manager_.append_solve_information("concentration", "picard_iterations", iteration + 1);
            output_manager_.append_solve_information("concentration", "final_concentration_increment_inf_norm_atom_per_m3", increment);
            return;
        }
    }
    throw std::runtime_error("稳态核素扩散 Picard 迭代未收敛");
}

void RadioactivityProblem::solve_transient(double time_step, double theta)
{
    if (time_step == 0.0)
    {
        boundary_manager_.update(properties_system_.field_data());
        mfem::Array<int> essential_true_dofs;
        boundary_manager_.apply_essential_conditions(concentration_field_.mfem_grid_function(), essential_true_dofs);
        return;
    }
    mfem::GridFunction& concentration = concentration_field_.mfem_grid_function();
    mfem::FiniteElementSpace* space = concentration.FESpace();
    mfem::Vector old_concentration(concentration);
    mfem::GridFunction trial_concentration(space);
    trial_concentration = concentration;

    mfem::Vector mass_action(space->GetVSize());
    capacity_form_.SpMat().Mult(old_concentration, mass_action);
    mfem::Vector fixed_right_hand_side(mass_action);
    fixed_right_hand_side /= time_step;
    if (theta < 1.0)
    {
        const PropertiesFieldData& old_properties = properties_system_.previous_field_data();
        diffusion_manager_.update(old_properties);
        source_manager_.update(old_properties);
        boundary_manager_.update(old_properties);
        mfem::BilinearForm old_diffusion(space);
        mfem::LinearForm old_source(space);
        diffusion_manager_.add_integrators(old_diffusion);
        boundary_manager_.add_integrators(old_diffusion);
        source_manager_.add_integrators(old_source);
        boundary_manager_.add_integrators(old_source);
        old_diffusion.Assemble();
        old_diffusion.Finalize();
        old_source.Assemble();
        mfem::Vector action(space->GetVSize());
        old_diffusion.SpMat().Mult(old_concentration, action);
        fixed_right_hand_side.Add(-(1.0 - theta), action);
        fixed_right_hand_side.Add(-(1.0 - theta) * decay_constant_, mass_action);
        fixed_right_hand_side.Add(1.0 - theta, old_source);
    }

    const auto& regions = fem_system_.mesh_data().region_attributes();
    const bool nonlinear = theta > 0.0 && std::any_of(regions.begin(), regions.end(), [this](int region)
    {
        return properties_system_.coupled_with_field("radioactivity.nuclide_diffusion_coef", region, "concentration");
    });
    const NonlinearSolverConfig& picard = solver_system_.nonlinear_config("concentration");
    const int max_iterations = nonlinear ? picard.max_iterations : 1;
    boundary_manager_.update(properties_system_.field_data());
    mfem::Array<int> essential_true_dofs;
    boundary_manager_.apply_essential_conditions(trial_concentration, essential_true_dofs);
    mfem::LinearForm new_source(space);
    if (theta > 0.0)
    {
        source_manager_.update(properties_system_.field_data());
        source_manager_.add_integrators(new_source);
        boundary_manager_.add_integrators(new_source);
        new_source.Assemble();
        fixed_right_hand_side.Add(theta, new_source);
    }

    for (int iteration = 0; iteration < max_iterations; ++iteration)
    {
        mfem::Vector previous_trial(trial_concentration);
        mfem::Vector right_hand_side(fixed_right_hand_side);
        std::unique_ptr<mfem::SparseMatrix> system_matrix;
        if (theta == 0.0)
        {
            system_matrix = std::make_unique<mfem::SparseMatrix>(capacity_form_.SpMat());
            *system_matrix *= 1.0 / time_step;
        }
        else
        {
            properties_system_.update_field_dependent_materials("concentration", {{"concentration", &trial_concentration}}, true);
            diffusion_manager_.update(properties_system_.field_data());
            mfem::BilinearForm new_diffusion(space);
            diffusion_manager_.add_integrators(new_diffusion);
            boundary_manager_.add_integrators(new_diffusion);
            new_diffusion.Assemble();
            new_diffusion.Finalize();
            // A = (1/dt + theta lambda) M + theta K_{n+1}, K 含 Robin 左端项。
            system_matrix.reset(mfem::Add(1.0 / time_step + theta * decay_constant_, capacity_form_.SpMat(), theta, new_diffusion.SpMat()));
        }
        // 本质边界始终取新时刻浓度, 旧浓度向量与旧属性保持不变。
        for (const int dof : essential_true_dofs)
        {
            system_matrix->EliminateRowCol(dof, trial_concentration[dof], right_hand_side);
        }
        solver_system_.solve_linear("concentration", *system_matrix, right_hand_side, trial_concentration, output_manager_);

        previous_trial -= trial_concentration;
        const double increment = previous_trial.Normlinf();
        if (!std::isfinite(increment))
        {
            throw std::runtime_error("瞬态核素扩散迭代产生非有限浓度");
        }
        if (!nonlinear || increment <= picard.absolute_tolerance + picard.relative_tolerance * trial_concentration.Normlinf())
        {
            concentration = trial_concentration;
            output_manager_.append_solve_information("concentration", "picard_iterations", iteration + 1);
            output_manager_.append_solve_information("concentration", "final_concentration_increment_inf_norm_atom_per_m3", increment);
            return;
        }
    }
    throw std::runtime_error("瞬态核素扩散 Picard 迭代未收敛");
}

}
