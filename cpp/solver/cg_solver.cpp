#include "solver/cg_solver.hpp"
#include "solver/solver_system.hpp"

#include <stdexcept>

namespace trimuse
{

void solve_cg(const LinearSolverConfig& config, const mfem::Operator& matrix,
              const mfem::Vector& right_hand_side, mfem::Vector& solution)
{
    mfem::CGSolver solver;
    solver.SetOperator(matrix);
    solver.SetRelTol(config.relative_tolerance);
    solver.SetAbsTol(config.relative_tolerance * right_hand_side.Norml2());
    solver.SetMaxIter(config.max_iterations);
    solver.SetPrintLevel(0);
    solver.iterative_mode = true;
    solver.Mult(right_hand_side, solution);
    if (!solver.GetConverged())
    {
        throw std::runtime_error("CG 线性求解未收敛");
    }
}

}
