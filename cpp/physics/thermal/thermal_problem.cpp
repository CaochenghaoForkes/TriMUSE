#include "physics/thermal/thermal_problem.hpp"

#include <chrono>
#include <cmath>
#include <stdexcept>

namespace trimuse
{


ThermalProblem::ThermalProblem(FemSystem& fem, FieldData& accepted_temperature, PropertiesSystem& properties,
    SolverSystem& solver, OutputManager& output, const std::string& problem_type,
    const std::vector<ThermalInitialConfig>& initial, const std::vector<ThermalDomainConfig>& sources,
    const std::vector<ThermalBoundaryConfig>& boundaries, const std::vector<ThermalInterfaceConfig>& interfaces) // 接收连续场
    : ThermalProblem(fem, TrialField(fem, accepted_temperature), properties, solver, output, problem_type, initial, sources, boundaries, interfaces) // 包装成 TrialField 调用第三个构造函数进行委托构造
{
}

ThermalProblem::ThermalProblem(FemSystem& fem, RegionFieldData& accepted_temperature, PropertiesSystem& properties,
    SolverSystem& solver, OutputManager& output, const std::string& problem_type,
    const std::vector<ThermalInitialConfig>& initial, const std::vector<ThermalDomainConfig>& sources,
    const std::vector<ThermalBoundaryConfig>& boundaries, const std::vector<ThermalInterfaceConfig>& interfaces) // 接收分域场
    : ThermalProblem(fem, TrialField(fem, accepted_temperature), properties, solver, output, problem_type, initial, sources, boundaries, interfaces) // 包装成 TrialField 调用第三个构造函数进行委托构造
{
}

ThermalProblem::ThermalProblem(FemSystem& fem, TrialField trial_temperature, PropertiesSystem& properties,
    SolverSystem& solver, OutputManager& output, const std::string& problem_type,
    const std::vector<ThermalInitialConfig>& initial, const std::vector<ThermalDomainConfig>& sources,
    const std::vector<ThermalBoundaryConfig>& boundaries, const std::vector<ThermalInterfaceConfig>& interfaces)
    : trial_temperature_(std::move(trial_temperature)), properties_(properties), solver_(solver), output_(output),
      transient_(problem_type == "transient"), assembly_(fem, trial_temperature_, sources, boundaries, interfaces) // trial_temperature_(std::move(trial_temperature)) 把传入试探场的资源移动给成员 trial_temperature_ assembly_(fem, trial_temperature_, sources, boundaries, interfaces) 创建组装器 让它使用成员试探场及模型配置
{
    assembly_.mark_coupled(properties_, transient_); // 登记热方程所需属性与温度之间的耦合关系
    if (trial_temperature_.regional())
    { 
        initialize_temperature(*trial_temperature_.accepted_region_field(), initial); // 分域场直接设置初温
    }
    else 
    { 
        initialize_temperature(fem, *trial_temperature_.accepted_continuous_field(), initial);  // 连续场直接设置初温或投影
    }
    trial_temperature_.load(); // 将正式场的初温复制到试探场
    assembly_.project_initial(); // 必要时投影初温，使分域场满足理想接触等连续约束
    trial_temperature_.accept(); // 把当前试探场的场值复制回正式场，接受本次求解结果
}

void ThermalProblem::accept_temperature() // 收敛后的属性与最终温度同步; 上一时间步快照仍由外部调度统一提交
{
    trial_temperature_.accept();
    properties_.update_field_dependent_materials("temperature", {{"temperature", trial_temperature_.view()}}, true);
}

void ThermalProblem::solve(double time_step, double theta) // 热问题的统一求解入口: 准备试探场、区分初始时刻与正常求解，并记录本次耗时和组装次数
{
    output_.clear_solve_information("temperature"); // 清空温度场上一次求解的统计信息
    const auto started = std::chrono::steady_clock::now(); // 记录开始时间
    const auto before = assembly_.counts(); // 复制求解前的累计组装次数
    trial_temperature_.load(); // 把正式温度场复制到试探场，作为本次求解的初始状态
    if (transient_ && time_step == 0.0) // 瞬态问题的零时间步用于处理初始状态
    {
        assembly_.update_boundary(properties_.field_data()); // 更新当前边界值，让试探温度满足约束，再提交到正式场
        assembly_.apply_initial_constraints();
        accept_temperature();
        return;
    }
    iterate(time_step, theta); // 执行稳态求解或瞬态推进，包括必要的 Picard 迭代
    output_.append_solve_information("temperature", "solve_elapsed_seconds", std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count()); // 记录从开始到此处的耗时，单位为秒
    for (const auto& [name, count] : assembly_.counts()) // 遍历求解后的累计组装次数
    {
        const auto old = before.find(name); // 查找求解前对应的累计值
        output_.append_solve_information("temperature", name + "_assemblies", count - (old == before.end() ? 0 : old->second)); // 本次组装次数 = 求解后累计次数 - 求解前累计次数
    }
}

void ThermalProblem::iterate(double time_step, double theta) // 准备旧时刻贡献，更新当前温度相关属性，形成方程并求解，直到 Picard 迭代收敛 处理 M\dot{T} + KT =F
{
    const mfem::Vector previous_step_values = trial_temperature_.values(); // 保存进入本次求解时的完整温度向量
    ThermalAssembly::State old; // 准备旧时刻的组装状态
    mfem::Vector old_rhs(trial_temperature_.size()); old_rhs = 0.0; // 准备旧时刻的右端贡献
    if (transient_ && theta < 1.0) // 只有是瞬态且 theta < 1 时才会需要旧时刻的 Kn Fn
    {
        old = assembly_.update(properties_.previous_field_data(), true); // 根据旧属性取得 M_n,K_n,F_n 这里 true 表示需要容量矩阵；内部仍按缓存判断是否重新组装
        old.conduction->Mult(previous_step_values, old_rhs);
        old_rhs *= -1.0;
        old_rhs += *old.load;
        old_rhs *= 1.0 - theta; // old_rhs = (1-\theta)(Fn-KnTn)
    }
    assembly_.update_boundary(properties_.field_data()); // 更新新时刻的边界值
    assembly_.apply_initial_constraints(); // 并施加到当前试探温度
    const bool nonlinear = (!transient_ || theta > 0.0) && assembly_.coupled(properties_, transient_); // 同时满足两个条件开启迭代(1.方程使用当前时刻系数(稳态或瞬态theta>0) 2.方程中的属性确实与正在求解的温度耦合)
    const auto& picard = solver_.nonlinear_config("temperature");
    for (int iteration = 0; iteration < (nonlinear ? picard.max_iterations : 1); ++iteration) // 线性问题只求解一次，非线性问题最多执行配置的迭代次数
    {
        mfem::Vector iteration_delta = trial_temperature_.values(), rhs(trial_temperature_.size()); // 保存本轮更新前的试探温度，用于之后判断收敛
        ThermalAssembly::State current;
        if (!transient_ || theta > 0.0) // 如果需要当前属性(稳态或瞬态theta>0)
        {
            properties_.update_field_dependent_materials("temperature", {{"temperature", trial_temperature_.view()}}, true); // 用当前试探温度更新相关材料属性
            current = assembly_.update(properties_.field_data(), transient_); // 取得本轮的 M,K,F，仅重组发生变化的贡献 稳态不需要容量项
        }
        std::unique_ptr<mfem::SparseMatrix> matrix;
        if (!transient_) // 稳态直接使用 KT = F
        {
            matrix = std::make_unique<mfem::SparseMatrix>(*current.conduction);
            rhs = *current.load;
        }
        else // 瞬态
        {
            // 保留原有 theta 离散: M_theta=(1-theta)M_n+theta M_{n+1}。
            std::unique_ptr<mfem::SparseMatrix> mixed_mass;
            const mfem::SparseMatrix* mass = theta == 0.0 ? old.mass.get() : current.mass.get(); // 对于热容矩阵 theta=0 使用 Mn theta=1 使用 Mn+1
            if (theta > 0.0 && theta < 1.0) // 0 < theta < 1
            {
                mixed_mass.reset(mfem::Add(1.0 - theta, *old.mass, theta, *current.mass)); // 构造混合矩阵
                mass = mixed_mass.get();
            }
            mass->Mult(previous_step_values, rhs);
            rhs /= time_step;
            rhs += old_rhs; // b = MTn/delta_t + (1-theta)(Fn-KnTn)
            if (theta > 0.0) // 若 theta>0
            {
                rhs.Add(theta, *current.load); // b = MTn/delta_t + (1-theta)(Fn-KnTn) + thetaFn+1
                matrix.reset(mfem::Add(1.0 / time_step, *mass, theta, *current.conduction)); // A=M/delta_t + thetaKn+1
            }
            else // 若 theta=0
            {
                matrix = std::make_unique<mfem::SparseMatrix>(*mass);
                *matrix *= 1.0 / time_step; // A = M/delta_t
            }
        }
        mfem::SparseMatrix system;
        mfem::Vector load, reduced_solution;
        assembly_.form_system(*matrix, rhs, system, load, reduced_solution); // 处理界面连续约束和 Dirichlet 条件，形成实际待求解系统
        solver_.solve_linear("temperature", system, load, reduced_solution, output_);
        assembly_.recover(reduced_solution); // 求出独立自由度温度，再通过 T=Pu 恢复到完整试探场
        const auto trial_values = trial_temperature_.values();
        iteration_delta -= trial_values; // 旧试探值 - 新试探值，用于收敛判据
        const double increment = iteration_delta.Normlinf(); // 计算相邻两轮温度变化
        if (!std::isfinite(increment)) 
        { 
            throw std::runtime_error("导热迭代产生非有限温度"); 
        }
        if (!nonlinear || increment <= picard.absolute_tolerance + picard.relative_tolerance * trial_values.Normlinf()) // 线性问题求解一次后直接接受；非线性问题满足该条件后：
        {
            accept_temperature(); // 把试探温度提交到正式场，并同步最终温度对应的材料属性，再记录迭代次数和增量
            output_.append_solve_information("temperature", "picard_iterations", iteration + 1);
            output_.append_solve_information("temperature", "final_temperature_increment_inf_norm_K", increment);
            return;
        }
    }
    throw std::runtime_error("导热 Picard 迭代未收敛");
}

}
