#include "solver/solver_system.hpp"
#include "solver/cg_solver.hpp"

#include <stdexcept>

namespace trimuse
{

SolverSystem::SolverSystem(const std::vector<FieldSolverConfig>& configs)
{
    for (const FieldSolverConfig& config : configs)
    {
        field_configs_.emplace(config.name, config);
    }
}

const NonlinearSolverConfig& SolverSystem::nonlinear_config(const std::string& field_name) const
{
    return field_configs_.at(field_name).nonlinear;
}

void SolverSystem::solve_linear(const std::string& field_name, const mfem::Operator& matrix, const mfem::Vector& right_hand_side, mfem::Vector& solution) const
{
    const LinearSolverConfig& config = field_configs_.at(field_name).linear;
    if (config.method != "cg" || config.preconditioner != "none")
    {
        throw std::invalid_argument("当前只支持无预条件的 CG 线性求解");
    }

    solve_cg(config, matrix, right_hand_side, solution);
}

}
