#include "solver/cg_solver.hpp"
#include "solver/solver_system.hpp"
#include "output/output_manager.hpp"

#include <chrono>
#include <stdexcept>

namespace trimuse
{

void solve_cg(const LinearSolverConfig& config, const mfem::Operator& matrix,
              const mfem::Vector& right_hand_side, mfem::Vector& solution,
              const std::string& field_name, OutputManager& output_manager)
{
    const auto started = std::chrono::steady_clock::now();
    mfem::CGSolver solver;
    solver.SetOperator(matrix);
    solver.SetRelTol(config.relative_tolerance);
    solver.SetAbsTol(config.relative_tolerance * right_hand_side.Norml2());
    solver.SetMaxIter(config.max_iterations);
    solver.SetPrintLevel(0);
    solver.iterative_mode = true;
    solver.Mult(right_hand_side, solution);
    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    output_manager.append_solve_information(field_name, "cg_iterations", solver.GetNumIterations());
    output_manager.append_solve_information(field_name, "cg_final_residual_norm", solver.GetFinalNorm());
    output_manager.append_solve_information(field_name, "cg_elapsed_seconds", elapsed);
    if (!solver.GetConverged())
    {
        throw std::runtime_error("CG 线性求解未收敛");
    }
}

}
