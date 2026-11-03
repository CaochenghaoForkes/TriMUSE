#pragma once

#include "physics/radioactivity/radioactivity_assembly.hpp"
#include "physics/radioactivity/initial/radioactivity_initial_condition.hpp"
#include "output/output_manager.hpp"
#include "solver/solver_system.hpp"

namespace trimuse
{
class RadioactivityProblem // 调度单核素浓度场的初值、稳态或瞬态求解及 Picard 迭代
{
public:
    RadioactivityProblem(FemSystem& fem, FieldData& accepted_concentration, PropertiesSystem& properties,
                   SolverSystem& solver, OutputManager& output, const std::string& problem_type, double decay_constant,
                   const std::vector<RadioactivityInitialConfig>& initial,
                   const std::vector<RadioactivityDomainConfig>& sources,
                   const std::vector<RadioactivityBoundaryConfig>& boundaries,
                   const std::vector<RadioactivityInterfaceConfig>& interfaces = {});
    RadioactivityProblem(FemSystem& fem, RegionFieldData& accepted_concentration, PropertiesSystem& properties,
                   SolverSystem& solver, OutputManager& output, const std::string& problem_type, double decay_constant,
                   const std::vector<RadioactivityInitialConfig>& initial,
                   const std::vector<RadioactivityDomainConfig>& sources,
                   const std::vector<RadioactivityBoundaryConfig>& boundaries,
                   const std::vector<RadioactivityInterfaceConfig>& interfaces = {});

    void solve(double time_step = 0.0, double theta = 1.0); // 单步求解入口；零时间步只处理瞬态初始约束
    const RadioactivityAssembly::Counts& assembly_counts() const { return assembly_.counts(); } // 返回各贡献累计组装次数

private:
    RadioactivityProblem(FemSystem& fem, TrialField trial_concentration, PropertiesSystem& properties,
                   SolverSystem& solver, OutputManager& output, const std::string& problem_type, double decay_constant,
                   const std::vector<RadioactivityInitialConfig>& initial,
                   const std::vector<RadioactivityDomainConfig>& sources,
                   const std::vector<RadioactivityBoundaryConfig>& boundaries,
                   const std::vector<RadioactivityInterfaceConfig>& interfaces);
    void iterate(double time_step, double theta); // 形成 M c_dot+Kc=F 的稳态或 theta 离散系统并求解
    void accept_concentration(); // 提交试探浓度并同步浓度依赖属性

    TrialField trial_concentration_; // 本次求解的试探浓度场，accept() 后才写回正式场
    PropertiesSystem& properties_; // 当前属性及上一时刻已接受的属性快照
    SolverSystem& solver_; // 线性求解器与 Picard 收敛参数
    OutputManager& output_; // 本步求解统计信息
    bool transient_; // true 为瞬态，false 为稳态
    RadioactivityAssembly assembly_; // 汇总 M、K、F 并处理约束
};
}
