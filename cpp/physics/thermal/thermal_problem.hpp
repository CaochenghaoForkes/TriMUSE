#pragma once

#include <string>
#include <vector>

#include "fem/fem_system.hpp"
#include "physics/thermal/boundary/thermal_boundary_manager.hpp"
#include "physics/thermal/initial/thermal_initial_condition.hpp"
#include "physics/thermal/source/thermal_source_manager.hpp"
#include "physics/thermal/thermal_capacity_manager.hpp"
#include "physics/thermal/thermal_conduction_manager.hpp"
#include "properties/properties_system.hpp"
#include "solver/solver_system.hpp"

namespace trimuse
{

class ThermalProblem
{
public:
    ThermalProblem(FemSystem& fem_system, FieldData& temperature_field, PropertiesSystem& properties_system,
                   SolverSystem& solver_system,
                   const std::string& problem_type,
                   const std::vector<ThermalInitialConfig>& initial_configs,
                   const std::vector<ThermalSourceConfig>& source_configs,
                   const std::vector<ThermalBoundaryConfig>& boundary_configs);

    void solve(double time_step = 0.0, double theta = 1.0);

private:
    void solve_steady();

    void solve_transient(double time_step, double theta);

    FemSystem& fem_system_;
    FieldData& temperature_field_;
    PropertiesSystem& properties_system_;
    SolverSystem& solver_system_;
    std::string problem_type_; // [steady,transient]
    ThermalSourceManager source_manager_;
    ThermalBoundaryManager boundary_manager_;
    ThermalConductionManager conduction_manager_;
    ThermalCapacityManager capacity_manager_;
};

}
