#pragma once

#include "physics/mechanics/mechanics_assembly.hpp"
#include "physics/mechanics/mechanics_state.hpp"
#include "output/output_manager.hpp"
#include "solver/solver_system.hpp"

namespace trimuse
{

class MechanicsProblem // 调度当前线弹性与热膨胀模型的准静态平衡求解，每次求解一次线性方程
{
public:
    MechanicsProblem(FemSystem& fem, FieldData& accepted_displacement, PropertiesSystem& properties,
        SolverSystem& solver, OutputManager& output,
        const std::vector<MechanicsDomainConfig>& body_forces = {},
        const std::vector<MechanicsBoundaryConfig>& boundaries = {},
        const std::vector<MechanicsInterfaceConfig>& interfaces = {});

    MechanicsProblem(FemSystem& fem, RegionFieldData& accepted_displacement, PropertiesSystem& properties,
        SolverSystem& solver, OutputManager& output,
        const std::vector<MechanicsDomainConfig>& body_forces = {},
        const std::vector<MechanicsBoundaryConfig>& boundaries = {},
        const std::vector<MechanicsInterfaceConfig>& interfaces = {});

    void solve(); // 按当前温度、属性和载荷求解平衡，提交位移与求积点状态，并记录求解信息

    void save_output(int cycle, double time, double time_step); // 保存当前正式状态的最大主应力和最大剪应力，单位 Pa

    const MechanicsState::Values& accepted_state() const; // 已接受的求积点总应变与应力，只读访问

    const MechanicsAssembly::Counts& assembly_counts() const;

private:
    MechanicsProblem(FemSystem& fem, TrialField trial_displacement, PropertiesSystem& properties,
        SolverSystem& solver, OutputManager& output,
        const std::vector<MechanicsDomainConfig>& body_forces,
        const std::vector<MechanicsBoundaryConfig>& boundaries,
        const std::vector<MechanicsInterfaceConfig>& interfaces);

    TrialField trial_displacement_; // 本次求解的独立试探位移，求解成功后才写回正式场
    PropertiesSystem& properties_;
    SolverSystem& solver_;
    OutputManager& output_;
    MechanicsAssembly assembly_; // 汇总刚度与载荷，处理界面、指定位移和刚体约束
    MechanicsState mechanics_state_; // 试探与已接受的求积点总应变、应力
};

}
