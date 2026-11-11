#include "physics/mechanics/mechanics_problem.hpp"

#include <chrono>
#include <utility>

namespace trimuse
{

MechanicsProblem::MechanicsProblem(FemSystem& fem, FieldData& accepted_displacement, PropertiesSystem& properties,
    SolverSystem& solver, OutputManager& output,
    const std::vector<MechanicsDomainConfig>& body_forces,
    const std::vector<MechanicsBoundaryConfig>& boundaries,
    const std::vector<MechanicsInterfaceConfig>& interfaces) // 接收连续场交给第三个构造函数委托构造
    : MechanicsProblem(fem, TrialField(fem, accepted_displacement), properties, solver, output, body_forces, boundaries, interfaces) 
{
}

MechanicsProblem::MechanicsProblem(FemSystem& fem, RegionFieldData& accepted_displacement, PropertiesSystem& properties,
    SolverSystem& solver, OutputManager& output,
    const std::vector<MechanicsDomainConfig>& body_forces,
    const std::vector<MechanicsBoundaryConfig>& boundaries,
    const std::vector<MechanicsInterfaceConfig>& interfaces) // 接收分域场生成试探场交给第三个构造函数委托构造
    : MechanicsProblem(fem, TrialField(fem, accepted_displacement), properties, solver, output, body_forces, boundaries, interfaces)
{
}

MechanicsProblem::MechanicsProblem(FemSystem& fem, TrialField trial_displacement, PropertiesSystem& properties,
    SolverSystem& solver, OutputManager& output,
    const std::vector<MechanicsDomainConfig>& body_forces,
    const std::vector<MechanicsBoundaryConfig>& boundaries,
    const std::vector<MechanicsInterfaceConfig>& interfaces) // 真正的构造函数，初始化各对象，标记耦合
    : trial_displacement_(std::move(trial_displacement)), properties_(properties), solver_(solver), output_(output), assembly_(fem, trial_displacement_, body_forces, boundaries, interfaces), mechanics_state_(fem.domain_quadrature_point_count())
{
    assembly_.mark_coupled(properties_);
}

void MechanicsProblem::solve()
{
    output_.clear_solve_information("displacement");
    const auto started = std::chrono::steady_clock::now();
    const auto before = assembly_.counts();
    trial_displacement_.load(); // 正式位移 -> 试探位移，作为本次求解的初始猜测
    properties_.update_field_dependent_materials("displacement", {{"displacement", trial_displacement_.view()}}, true);
    const auto current = assembly_.update(properties_.field_data()); // 更新系数，按缓存取得完整刚度与总载荷

    mfem::SparseMatrix system;
    mfem::Vector load, reduced_displacement;
    assembly_.form_system(*current.stiffness, *current.load, system, load, reduced_displacement);
    solver_.solve_linear("displacement", system, load, reduced_displacement, output_);
    assembly_.recover(reduced_displacement); // 独立位移 -> 完整试探位移
    assembly_.update_state(mechanics_state_); // 用同一次组装的 C、beta 和新位移恢复试探总应变、应力
    trial_displacement_.accept();
    mechanics_state_.accept(); // 位移与求积点状态一起提交

    output_.append_solve_information("displacement", "solve_elapsed_seconds", std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count());
    for (const auto& [name, count] : assembly_.counts())
    {
        const auto old = before.find(name);
        output_.append_solve_information("displacement", name + "_assemblies", count - (old == before.end() ? 0 : old->second));
    }
}

const MechanicsState::Values& MechanicsProblem::accepted_state() const
{
    return mechanics_state_.accepted_values();
}

const MechanicsAssembly::Counts& MechanicsProblem::assembly_counts() const
{
    return assembly_.counts();
}

}
