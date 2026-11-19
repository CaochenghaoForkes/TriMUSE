#include "physics/mechanics/mechanics_problem.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

namespace trimuse
{

MechanicsProblem::MechanicsProblem(FemSystem& fem, FieldData& accepted_displacement, PropertiesSystem& properties, SolverSystem& solver, OutputManager& output, const MechanicsRegionConfigs& regions, const MechanicsBoundaryConfigs& boundaries, const MechanicsInterfaceConfigs& interfaces) // 接收连续场交给第三个构造函数委托构造
    : MechanicsProblem(fem, TrialField(fem, accepted_displacement), properties, solver, output, regions, boundaries, interfaces)
{
}

MechanicsProblem::MechanicsProblem(FemSystem& fem, RegionFieldData& accepted_displacement, PropertiesSystem& properties, SolverSystem& solver, OutputManager& output, const MechanicsRegionConfigs& regions, const MechanicsBoundaryConfigs& boundaries, const MechanicsInterfaceConfigs& interfaces) // 接收分域场生成试探场交给第三个构造函数委托构造
    : MechanicsProblem(fem, TrialField(fem, accepted_displacement), properties, solver, output, regions, boundaries, interfaces)
{
}

MechanicsProblem::MechanicsProblem(FemSystem& fem, TrialField trial_displacement, PropertiesSystem& properties, SolverSystem& solver, OutputManager& output, const MechanicsRegionConfigs& regions, const MechanicsBoundaryConfigs& boundaries, const MechanicsInterfaceConfigs& interfaces) // 真正的构造函数，初始化各对象，标记耦合，为输出对象添加待输出场值信息
    : trial_displacement_(std::move(trial_displacement)), properties_(properties), solver_(solver), output_(output), assembly_(fem, trial_displacement_, regions, boundaries, interfaces), mechanics_state_(fem.domain_quadrature_point_count())
{
    assembly_.mark_coupled(properties_);
    const int region = fem.mesh_data().region_attributes().front();
    const int order = std::max(0, trial_displacement_.space(region).GetMaxElementOrder() - 1); // 应力采用逐单元 L2 空间，保留材料界面跳变
    output_.add_field("max_principal_stress", "mechanics", true, order);
    output_.add_field("max_shear_stress", "mechanics", true, order);
}

void MechanicsProblem::solve()
{
    output_.clear_solve_information("displacement");
    const auto started = std::chrono::steady_clock::now();
    const auto before = assembly_.counts();
    trial_displacement_.load(); // 正式位移 -> 试探位移，作为本次求解的初始猜测
    const PropertiesSystem::FieldMap trial_fields{{"displacement", trial_displacement_.view()}};
    properties_.update_field_dependent_materials("displacement", trial_fields, true);
    properties_.update_field_dependent_assembly("displacement", trial_fields, true);
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

void MechanicsProblem::save_output(int cycle, double time, double time_step)
{
    const auto& state = mechanics_state_.accepted_values();
    std::vector<double> maximum(state.principal_stress.size());
    for (std::size_t point = 0; point < maximum.size(); ++point)
    {
        maximum[point] = state.principal_stress[point][0];
    }
    output_.save("max_principal_stress", maximum, cycle, time, time_step);
    output_.save("max_shear_stress", state.max_shear_stress, cycle, time, time_step);
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
