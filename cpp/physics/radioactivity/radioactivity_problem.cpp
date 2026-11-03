#include "physics/radioactivity/radioactivity_problem.hpp"

#include <chrono>
#include <cmath>
#include <stdexcept>

namespace trimuse
{

RadioactivityProblem::RadioactivityProblem(FemSystem& fem, FieldData& accepted_concentration, PropertiesSystem& properties,
    SolverSystem& solver, OutputManager& output, const std::string& problem_type, double decay_constant,
    const std::vector<RadioactivityInitialConfig>& initial, const std::vector<RadioactivityDomainConfig>& sources,
    const std::vector<RadioactivityBoundaryConfig>& boundaries, const std::vector<RadioactivityInterfaceConfig>& interfaces)
    : RadioactivityProblem(fem, TrialField(fem, accepted_concentration), properties, solver, output, problem_type, decay_constant, initial, sources, boundaries, interfaces) // 连续正式场包装为独立试探场，再交给统一构造函数
{
}
RadioactivityProblem::RadioactivityProblem(FemSystem& fem, RegionFieldData& accepted_concentration, PropertiesSystem& properties,
    SolverSystem& solver, OutputManager& output, const std::string& problem_type, double decay_constant,
    const std::vector<RadioactivityInitialConfig>& initial, const std::vector<RadioactivityDomainConfig>& sources,
    const std::vector<RadioactivityBoundaryConfig>& boundaries, const std::vector<RadioactivityInterfaceConfig>& interfaces)
    : RadioactivityProblem(fem, TrialField(fem, accepted_concentration), properties, solver, output, problem_type, decay_constant, initial, sources, boundaries, interfaces) // 分域正式场同样包装为试探场
{
}
RadioactivityProblem::RadioactivityProblem(FemSystem& fem, TrialField trial_concentration, PropertiesSystem& properties,
    SolverSystem& solver, OutputManager& output, const std::string& problem_type, double decay_constant,
    const std::vector<RadioactivityInitialConfig>& initial, const std::vector<RadioactivityDomainConfig>& sources,
    const std::vector<RadioactivityBoundaryConfig>& boundaries, const std::vector<RadioactivityInterfaceConfig>& interfaces)
    : trial_concentration_(std::move(trial_concentration)), properties_(properties), solver_(solver), output_(output),
      transient_(problem_type == "transient"), assembly_(fem, trial_concentration_, decay_constant, sources, boundaries, interfaces)
{
    assembly_.mark_coupled(properties_); // 登记扩散、源和边界所需属性与浓度场的耦合关系
    if (trial_concentration_.regional()) { initialize_concentration(fem, *trial_concentration_.accepted_region_field(), initial); }
    else { initialize_concentration(fem, *trial_concentration_.accepted_continuous_field(), initial); }
    trial_concentration_.load(); // 正式场初值复制到试探场
    assembly_.project_initial(); // 必要时将区域初始阶跃投影到完美接触约束空间
    trial_concentration_.accept(); // 将投影后的初值写回正式场
}

void RadioactivityProblem::accept_concentration()
{
    trial_concentration_.accept(); // 当前试探解写入 FemSystem 所持有的正式浓度场
    // 收敛后的属性与最终浓度同步; 上一时间步快照仍由外部调度统一提交。
    properties_.update_field_dependent_materials("concentration", {{"concentration", trial_concentration_.view()}}, true);
}

void RadioactivityProblem::solve(double time_step, double theta) // 每个时刻的统一入口，负责初始状态、正常求解与组装统计
{
    output_.clear_solve_information("concentration");
    const auto started = std::chrono::steady_clock::now();
    const auto before = assembly_.counts(); // 用于计算本步新增的组装次数
    trial_concentration_.load(); // 以正式浓度作为本步试探初值
    if (transient_ && time_step == 0.0) // t=0 只施加当前 Dirichlet 值，不推进时间
    {
        assembly_.update_boundary(properties_.field_data());
        assembly_.apply_initial_constraints();
        accept_concentration(); // 接受零时刻浓度并同步材料属性
        return;
    }
    iterate(time_step, theta); // 稳态求解或瞬态时间推进
    output_.append_solve_information("concentration", "solve_elapsed_seconds",
        std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count());
    for (const auto& [name, count] : assembly_.counts())
    {
        const auto old = before.find(name);
        output_.append_solve_information("concentration", name + "_assemblies",
            count - (old == before.end() ? 0 : old->second));
    }
}

void RadioactivityProblem::iterate(double time_step, double theta) // 在完整自由度上形成方程，再约化并求解
{
    const mfem::Vector previous_step_values = trial_concentration_.values(); // 保存已接受的 c_n
    RadioactivityAssembly::State old; // 旧时刻的只读 M、K、F
    mfem::Vector old_rhs(trial_concentration_.size()); old_rhs = 0.0; // 旧时刻显式贡献
    if (transient_ && theta < 1.0) // theta=1 时不需要旧时刻的 K_n 和 F_n
    {
        old = assembly_.update(properties_.previous_field_data()); // 按 t_n 已接受属性取得旧矩阵和右端
        old.diffusion->Mult(previous_step_values, old_rhs); // K_n c_n；K 已含扩散、Robin 和衰变
        old_rhs *= -1.0; // -K_n c_n
        old_rhs += *old.load; // F_n-K_n c_n
        old_rhs *= 1.0 - theta; // (1-theta)(F_n-K_n c_n)
    }
    // 在新时刻施加本质边界, 不修改旧浓度或已缓存的矩阵。
    assembly_.update_boundary(properties_.field_data());
    assembly_.apply_initial_constraints();
    const bool nonlinear = (!transient_ || theta > 0.0) && assembly_.coupled(properties_); // 当前时刻系数参与方程且依赖浓度时才迭代
    const auto& picard = solver_.nonlinear_config("concentration");
    for (int iteration = 0; iteration < (nonlinear ? picard.max_iterations : 1); ++iteration) // 线性问题只求解一次
    {
        mfem::Vector iteration_delta = trial_concentration_.values(), rhs(trial_concentration_.size()); // 保存本轮求解前的试探值
        RadioactivityAssembly::State current; // 当前时刻的只读 M、K、F
        if (!transient_ || theta > 0.0) // 稳态或隐式比例非零时才需要当前时刻贡献
        {
            properties_.update_field_dependent_materials("concentration", {{"concentration", trial_concentration_.view()}}, true); // 用当前试探浓度更新耦合材料属性
            current = assembly_.update(properties_.field_data()); // 按需重组变化区域，形成当前 M、K、F
        }
        std::unique_ptr<mfem::SparseMatrix> matrix;
        if (!transient_)
        {
            // (Kdiffusion + KRobin + lambda M)c = F。
            matrix = std::make_unique<mfem::SparseMatrix>(*current.diffusion); // 稳态 A=K
            rhs = *current.load; // 稳态 b=F
        }
        else
        {
            // M 固定；K 已包含 lambda M，theta 离散统一作用于扩散、衰变和边界项。
            const auto& mass = theta == 0.0 ? old.mass : current.mass; // M 恒定；选已取得的旧或当前状态引用
            mass->Mult(previous_step_values, rhs); // M c_n
            rhs /= time_step; // M c_n / delta_t
            rhs += old_rhs; // 加上 (1-theta)(F_n-K_n c_n)
            if (theta > 0.0)
            {
                rhs.Add(theta, *current.load); // b=M c_n/delta_t+(1-theta)(F_n-K_n c_n)+theta F_{n+1}
                matrix.reset(mfem::Add(1.0 / time_step, *mass, theta, *current.diffusion)); // A=M/delta_t+theta K_{n+1}
            }
            else
            {
                matrix = std::make_unique<mfem::SparseMatrix>(*mass);
                *matrix *= 1.0 / time_step; // theta=0 时 A=M/delta_t
            }
        }
        mfem::SparseMatrix system;
        mfem::Vector load, reduced_solution;
        assembly_.form_system(*matrix, rhs, system, load, reduced_solution); // 完美接触约化和 Dirichlet 消元
        solver_.solve_linear("concentration", system, load, reduced_solution, output_); // 求独立自由度浓度
        assembly_.recover(reduced_solution); // 通过 P 写入完整试探浓度场
        const auto trial_values = trial_concentration_.values();
        iteration_delta -= trial_values; // 旧试探值 - 新试探值，用于收敛判据
        const double increment = iteration_delta.Normlinf(); // 相邻两轮试探浓度的最大差值
        if (!std::isfinite(increment)) { throw std::runtime_error("核素扩散迭代产生非有限浓度"); }
        if (!nonlinear || increment <= picard.absolute_tolerance + picard.relative_tolerance * trial_values.Normlinf())
        {
            accept_concentration(); // 满足 Picard 判据后提交正式场
            output_.append_solve_information("concentration", "picard_iterations", iteration + 1);
            output_.append_solve_information("concentration", "final_concentration_increment_inf_norm_atom_per_m3", increment);
            return;
        }
    }
    throw std::runtime_error("核素扩散 Picard 迭代未收敛");
}
}
