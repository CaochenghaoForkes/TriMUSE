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
    MechanicsProblem(FemSystem& fem, RegionFieldData& accepted_displacement, PropertiesSystem& properties, SolverSystem& solver, OutputManager& output_manager, const MechanicsRegionConfigs& regions, const MechanicsBoundaryConfigs& boundaries, const MechanicsInterfaceConfigs& interfaces);
    void solve(); // 按当前温度、属性和载荷求解平衡，提交位移与求积点状态，并记录求解信息
    const MechanicsState::Values& accepted_state() const; // 已接受的求积点总应变与应力，只读访问
    const MechanicsAssembly::Counts& assembly_counts() const;

private:
    TrialField trial_displacement_; // 本次求解的独立试探位移，求解成功后才写回正式场
    PropertiesSystem& properties_;
    SolverSystem& solver_;
    OutputManager& output_manager_;
    MechanicsAssembly assembly_; // 汇总刚度与载荷，处理界面、指定位移和刚体约束
    MechanicsState mechanics_state_; // 试探与已接受的求积点总应变、应力
};

}
