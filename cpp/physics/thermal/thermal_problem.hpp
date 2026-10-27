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
    ThermalProblem(FemSystem& fem, FieldData& temperature, PropertiesSystem& properties,
                   SolverSystem& solver, OutputManager& output, const std::string& problem_type,
                   const std::vector<ThermalInitialConfig>& initial,
                   const std::vector<ThermalDomainConfig>& sources,
                   const std::vector<ThermalBoundaryConfig>& boundaries,
                   const std::vector<ThermalInterfaceConfig>& interfaces = {});
    ThermalProblem(FemSystem& fem, RegionFieldData& temperature, PropertiesSystem& properties,
                   SolverSystem& solver, OutputManager& output, const std::string& problem_type,
                   const std::vector<ThermalInitialConfig>& initial,
                   const std::vector<ThermalDomainConfig>& sources,
                   const std::vector<ThermalBoundaryConfig>& boundaries,
                   const std::vector<ThermalInterfaceConfig>& interfaces = {});

    void solve(double time_step = 0.0, double theta = 1.0);
    const ThermalAssembly::Counts& assembly_counts() const { return assembly_.counts(); }

private:
    ThermalProblem(FemSystem& fem, ThermalField temperature, PropertiesSystem& properties,
                   SolverSystem& solver, OutputManager& output, const std::string& problem_type,
                   const std::vector<ThermalInitialConfig>& initial,
                   const std::vector<ThermalDomainConfig>& sources,
                   const std::vector<ThermalBoundaryConfig>& boundaries,
                   const std::vector<ThermalInterfaceConfig>& interfaces);
    void iterate(double time_step, double theta);
    void accept_temperature();

    ThermalField temperature_;
    PropertiesSystem& properties_;
    SolverSystem& solver_;
    OutputManager& output_;
    bool transient_;
    ThermalAssembly assembly_;
};
}
