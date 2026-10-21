#include "physics/thermal/thermal_problem.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace trimuse
{

ThermalProblem::ThermalProblem(FemSystem& fem_system, FieldData& temperature_field, PropertiesSystem& properties_system,
                               SolverSystem& solver_system,
                               const std::string& problem_type,
                               const std::vector<ThermalInitialConfig>& initial_configs,
                               const std::vector<ThermalSourceConfig>& source_configs,
                               const std::vector<ThermalBoundaryConfig>& boundary_configs)
    : fem_system_(fem_system),
      temperature_field_(temperature_field),
      properties_system_(properties_system),
      solver_system_(solver_system),
      problem_type_(problem_type),
      source_manager_(fem_system, source_configs),
      boundary_manager_(fem_system, boundary_configs),
      conduction_manager_(fem_system),
      capacity_manager_(fem_system)
{
    if (problem_type_ == "steady")
    {
        properties_system_.mark_coupled("temperature", {"thermal.conductivity"});
    }
    else
    {
        properties_system_.mark_coupled("temperature", {"general.mass_density", "thermal.specific_heat", "thermal.conductivity"});
    }
    trimuse::initialize_temperature(fem_system_, temperature_field_, initial_configs);
}

void ThermalProblem::solve(double time_step, double theta)
{
    if (problem_type_ == "steady")
    {
        solve_steady();
    }
    else
    {
        solve_transient(time_step, theta);
    }
}

void ThermalProblem::solve_steady()
{
    mfem::GridFunction& temperature = temperature_field_.mfem_grid_function();
    mfem::FiniteElementSpace* space = temperature.FESpace();
    mfem::GridFunction trial_temperature(space);
    trial_temperature = temperature;
    const auto& regions = fem_system_.mesh_data().region_attributes();
    const bool nonlinear = std::any_of(regions.begin(), regions.end(), [this](int region)
    {
        return properties_system_.coupled_with_field("thermal.conductivity", region, "temperature");
    });
    const NonlinearSolverConfig& picard = solver_system_.nonlinear_config("temperature");
    const int max_iterations = nonlinear ? picard.max_iterations : 1;
    source_manager_.update(properties_system_.field_data());
    boundary_manager_.update(properties_system_.field_data());
    mfem::Array<int> essential_true_dofs;
    boundary_manager_.apply_essential_conditions(trial_temperature, essential_true_dofs); // 1.把指定表面温度写入 trial_temperature 对应的边界自由度 2.把这些需要约束的真实自由度编号填入 essential_true_dofs

    for (int iteration = 0; iteration < max_iterations; ++iteration)
    {
        mfem::Vector previous_temperature(trial_temperature); // Picard 前一轮解
        properties_system_.update_field_dependent_materials("temperature", {{"temperature", &trial_temperature}}, true);
        conduction_manager_.update(properties_system_.field_data());

        mfem::BilinearForm conduction_form(space);
        mfem::LinearForm source_form(space);
        conduction_manager_.add_integrators(conduction_form);
        source_manager_.add_integrators(source_form);
        boundary_manager_.add_integrators(conduction_form, source_form);
        conduction_form.Assemble();
        source_form.Assemble();

        mfem::SparseMatrix system_matrix;
        mfem::Vector true_temperature, true_right_hand_side;
        conduction_form.FormLinearSystem(essential_true_dofs, trial_temperature, source_form, system_matrix, true_temperature, true_right_hand_side, 1);
        solver_system_.solve_linear("temperature", system_matrix, true_right_hand_side, true_temperature);
        conduction_form.RecoverFEMSolution(true_temperature, source_form, trial_temperature);

        previous_temperature -= trial_temperature;
        const double increment = previous_temperature.Normlinf();
        if (!std::isfinite(increment))
        {
            throw std::runtime_error("稳态导热迭代产生非有限温度");
        }
        if (!nonlinear || increment <= picard.absolute_tolerance + picard.relative_tolerance * trial_temperature.Normlinf())
        {
            temperature = trial_temperature;
            return;
        }
    }
    throw std::runtime_error("稳态导热 Picard 迭代未收敛");
}

void ThermalProblem::solve_transient(double time_step, double theta)
{
    // 初始时刻施加指定边界温度, 不执行时间推进。
    if (time_step == 0.0)
    {
        boundary_manager_.update(properties_system_.field_data());
        mfem::Array<int> essential_true_dofs;
        boundary_manager_.apply_essential_conditions(temperature_field_.mfem_grid_function(), essential_true_dofs);
        return;
    }
    mfem::GridFunction& temperature = temperature_field_.mfem_grid_function();
    mfem::FiniteElementSpace* space = temperature.FESpace();
    mfem::Vector old_temperature(temperature);
    mfem::GridFunction trial_temperature(space);
    trial_temperature = temperature;

    // 上一步的属性快照固定不变。K 同时包含域内导热和 Robin 左端项,
    // F 同时包含体积热源、Robin 和 Neumann 右端项。
    const PropertiesFieldData& old_properties = properties_system_.previous_field_data();
    capacity_manager_.update(old_properties);
    conduction_manager_.update(old_properties);
    source_manager_.update(old_properties);
    boundary_manager_.update(old_properties);
    mfem::BilinearForm old_capacity(space), old_conduction(space);
    mfem::LinearForm old_source(space);
    capacity_manager_.add_integrators(old_capacity);
    conduction_manager_.add_integrators(old_conduction);
    source_manager_.add_integrators(old_source);
    boundary_manager_.add_integrators(old_conduction, old_source);
    old_capacity.Assemble();
    old_conduction.Assemble();
    old_source.Assemble();
    old_capacity.Finalize();
    old_conduction.Finalize();

    const auto& regions = fem_system_.mesh_data().region_attributes();
    const bool nonlinear = theta > 0.0 && std::any_of(regions.begin(), regions.end(), [this](int region)
    {
        return properties_system_.coupled_with_field("general.mass_density", region, "temperature") ||
               properties_system_.coupled_with_field("thermal.specific_heat", region, "temperature") ||
               properties_system_.coupled_with_field("thermal.conductivity", region, "temperature");
    });
    const NonlinearSolverConfig& picard = solver_system_.nonlinear_config("temperature");
    const int max_iterations = nonlinear ? picard.max_iterations : 1;

    if (theta > 0.0) // 新时刻的自然边界和源仅在 theta>0 时参与组装。
    {
        source_manager_.update(properties_system_.field_data());
    }
    // 无论 theta 取何值, 本质边界均约束为新时刻的指定温度; 旧温度向量保持不变。
    boundary_manager_.update(properties_system_.field_data());
    mfem::Array<int> essential_true_dofs;
    boundary_manager_.apply_essential_conditions(trial_temperature, essential_true_dofs);
    for (int iteration = 0; iteration < max_iterations; ++iteration)
    {
        mfem::Vector previous_trial(trial_temperature);
        mfem::Vector right_hand_side(space->GetVSize());
        mfem::Vector action(space->GetVSize());
        std::unique_ptr<mfem::SparseMatrix> system_matrix;

        if (theta == 0.0)
        {
            old_capacity.SpMat().Mult(old_temperature, right_hand_side); // SpMat() 取出已装配的旧热容矩阵 计算 MnTn 写入 right_hand_side
            right_hand_side /= time_step; // 右端变成 MnTn/delta_t
            system_matrix = std::make_unique<mfem::SparseMatrix>(old_capacity.SpMat()); // 取 Mn
            *system_matrix *= 1.0 / time_step; // 左端矩阵为 Mn/delta_t
        }
        else
        {
            properties_system_.update_field_dependent_materials("temperature", {{"temperature", &trial_temperature}}, true);
            const PropertiesFieldData& new_properties = properties_system_.field_data();
            capacity_manager_.update(new_properties);
            conduction_manager_.update(new_properties);
            mfem::BilinearForm new_capacity(space), new_conduction(space);
            mfem::LinearForm new_source(space);
            capacity_manager_.add_integrators(new_capacity);
            conduction_manager_.add_integrators(new_conduction);
            source_manager_.add_integrators(new_source);
            boundary_manager_.add_integrators(new_conduction, new_source);
            new_capacity.Assemble();
            new_conduction.Assemble();
            new_source.Assemble();
            new_capacity.Finalize();
            new_conduction.Finalize();

            std::unique_ptr<mfem::SparseMatrix> capacity_matrix(mfem::Add(1.0 - theta, old_capacity.SpMat(), theta, new_capacity.SpMat())); // M_theta = (1-theta) M_n + theta M_{n+1}
            capacity_matrix->Mult(old_temperature, right_hand_side); // right_hand_side = M_thetaTn 其中 M_theta=(1-theta)Mn+thetaMn+1
            right_hand_side /= time_step; // right_hand_side = M_thetaTn/delta_t
            right_hand_side.Add(theta, new_source); // right_hand_side = M_thetaTn/delta_t + theta*Fn+1
            system_matrix.reset(mfem::Add(1.0 / time_step, *capacity_matrix, theta, new_conduction.SpMat())); // A=M_theta/delta_t + thetaKn+1
        }

        old_conduction.SpMat().Mult(old_temperature, action); // action=KnTn
        right_hand_side.Add(-(1.0 - theta), action); // right_hand_side = right_hand_side - (1-theta)KnTn
        right_hand_side.Add(1.0 - theta, old_source); // right_hand_side = right_hand_side - (1-theta)KnTn + (1-theta)Fn
        // 当前为共形串行 H1 空间, 总体自由度与真实自由度一致; 对最终矩阵对称消元。
        for (const int dof : essential_true_dofs)
        {
            system_matrix->EliminateRowCol(dof, trial_temperature[dof], right_hand_side); // 在方程 AT=b 中，把编号为 dof 的温度固定为 trial_temperature[dof]
        }
        solver_system_.solve_linear("temperature", *system_matrix, right_hand_side, trial_temperature);

        previous_trial -= trial_temperature;
        const double increment = previous_trial.Normlinf();
        if (!std::isfinite(increment))
        {
            throw std::runtime_error("瞬态导热迭代产生非有限温度");
        }
        if (!nonlinear || increment <= picard.absolute_tolerance + picard.relative_tolerance * trial_temperature.Normlinf())
        {
            temperature = trial_temperature;
            return;
        }
    }
    throw std::runtime_error("瞬态导热 Picard 迭代未收敛");
}

}
