#pragma once

#include "physics/radioactivity/radioactivity_assembly.hpp"
#include "physics/radioactivity/initial/radioactivity_initial_condition.hpp"
#include "output/output_manager.hpp"
#include "solver/solver_system.hpp"

namespace trimuse
{
class RadioactivityProblem
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

    void solve(double time_step = 0.0, double theta = 1.0);
    const RadioactivityAssembly::Counts& assembly_counts() const { return assembly_.counts(); }

private:
    RadioactivityProblem(FemSystem& fem, TrialField trial_concentration, PropertiesSystem& properties,
                   SolverSystem& solver, OutputManager& output, const std::string& problem_type, double decay_constant,
                   const std::vector<RadioactivityInitialConfig>& initial,
                   const std::vector<RadioactivityDomainConfig>& sources,
                   const std::vector<RadioactivityBoundaryConfig>& boundaries,
                   const std::vector<RadioactivityInterfaceConfig>& interfaces);
    void iterate(double time_step, double theta);
    void accept_concentration();

    TrialField trial_concentration_; // 本次求解的试探浓度场，accept() 后才写回正式场
    PropertiesSystem& properties_;
    SolverSystem& solver_;
    OutputManager& output_;
    bool transient_;
    RadioactivityAssembly assembly_;
};
}
