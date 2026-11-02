#pragma once

#include "physics/thermal/thermal_assembly.hpp"
#include "physics/thermal/initial/thermal_initial_condition.hpp"
#include "output/output_manager.hpp"
#include "solver/solver_system.hpp"

namespace trimuse
{

class ThermalProblem
{
public:
    ThermalProblem(FemSystem& fem, FieldData& accepted_temperature, PropertiesSystem& properties, SolverSystem& solver, OutputManager& output, const std::string& problem_type,
                   const std::vector<ThermalInitialConfig>& initial,
                   const std::vector<ThermalDomainConfig>& sources,
                   const std::vector<ThermalBoundaryConfig>& boundaries,
                   const std::vector<ThermalInterfaceConfig>& interfaces = {});
    
    ThermalProblem(FemSystem& fem, RegionFieldData& accepted_temperature, PropertiesSystem& properties, SolverSystem& solver, OutputManager& output, const std::string& problem_type,
                    const std::vector<ThermalInitialConfig>& initial,
                    const std::vector<ThermalDomainConfig>& sources,
                    const std::vector<ThermalBoundaryConfig>& boundaries,
                    const std::vector<ThermalInterfaceConfig>& interfaces = {});

    void solve(double time_step = 0.0, double theta = 1.0); // // 热问题的统一求解入口: 准备试探场、区分初始时刻与正常求解，并记录本次耗时和组装次数

    const ThermalAssembly::Counts& assembly_counts() const { return assembly_.counts(); }

private:
    ThermalProblem(FemSystem& fem, TrialField trial_temperature, PropertiesSystem& properties, SolverSystem& solver, OutputManager& output, const std::string& problem_type,
                   const std::vector<ThermalInitialConfig>& initial,
                   const std::vector<ThermalDomainConfig>& sources,
                   const std::vector<ThermalBoundaryConfig>& boundaries,
                   const std::vector<ThermalInterfaceConfig>& interfaces);

    void iterate(double time_step, double theta); // // 准备旧时刻贡献，更新当前温度相关属性，形成方程并求解，直到 Picard 迭代收敛 处理 M\dot{T} + KT =F

    void accept_temperature(); // // 收敛后的属性与最终温度同步; 上一时间步快照仍由外部调度统一提交

    TrialField trial_temperature_; // 试探场

    PropertiesSystem& properties_;

    SolverSystem& solver_;

    OutputManager& output_;

    bool transient_;
    
    ThermalAssembly assembly_;
};

}
