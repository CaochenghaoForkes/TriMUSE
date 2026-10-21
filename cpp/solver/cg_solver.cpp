#include "solver/cg_solver.hpp"
#include "solver/solver_system.hpp"
#include "output/output_manager.hpp"

#include <chrono>
#include <memory>
#include <stdexcept>

namespace trimuse
{

void solve_cg(const LinearSolverConfig& config, const mfem::Operator& matrix,
              const mfem::Vector& right_hand_side, mfem::Vector& solution,
              const std::string& field_name, OutputManager& output_manager)
{
    const auto started = std::chrono::steady_clock::now();
    std::unique_ptr<mfem::Solver> preconditioner;
    if (config.preconditioner == "jacobi")
    {
        preconditioner = std::make_unique<mfem::DSmoother>();
    }
    else if (config.preconditioner == "symmetric_gauss_seidel")
    {
        preconditioner = std::make_unique<mfem::GSSmoother>(0); // 0 为对称模式
    }
    else if (config.preconditioner != "none")
    {
        throw std::invalid_argument("未知 CG 预条件器: " + config.preconditioner);
    }

    mfem::CGSolver solver;
    solver.SetOperator(matrix);
    if (preconditioner)
    {
        preconditioner->SetOperator(matrix); // 把 Ax=b 的 A 交给预条件器,使其能根据 A 执行预条件操作
        solver.SetPreconditioner(*preconditioner); // 把预条件器交给 CG,后续每轮迭代 CG 调用它转换残差
    }
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
