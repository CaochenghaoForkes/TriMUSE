#pragma once

#include <string>
#include <vector>

#include "fem/fem_system.hpp"
#include "output/output_manager.hpp"
#include "physics/radioactivity/boundary/radioactivity_boundary_manager.hpp"
#include "physics/radioactivity/initial/radioactivity_initial_condition.hpp"
#include "physics/radioactivity/source/radioactivity_source_manager.hpp"
#include "physics/radioactivity/radioactivity_capacity_manager.hpp"
#include "physics/radioactivity/radioactivity_diffusion_manager.hpp"
#include "properties/properties_system.hpp"
#include "solver/solver_system.hpp"

namespace trimuse
{

class RadioactivityProblem
{
public:
    RadioactivityProblem(FemSystem& fem_system, FieldData& concentration_field, PropertiesSystem& properties_system,
                         SolverSystem& solver_system, OutputManager& output_manager,
                         const std::string& problem_type, double decay_constant,
                         const std::vector<RadioactivityInitialConfig>& initial_configs,
                         const std::vector<RadioactivitySourceConfig>& source_configs,
                         const std::vector<RadioactivityBoundaryConfig>& boundary_configs);

    void solve(double time_step = 0.0, double theta = 1.0);

private:
    void solve_steady();

    void solve_transient(double time_step, double theta);

    FemSystem& fem_system_;
    FieldData& concentration_field_;
    PropertiesSystem& properties_system_;
    SolverSystem& solver_system_;
    OutputManager& output_manager_;
    std::string problem_type_;
    double decay_constant_; // s⁻¹, 一张卡只处理一个核素
    RadioactivitySourceManager source_manager_;
    RadioactivityBoundaryManager boundary_manager_;
    RadioactivityDiffusionManager diffusion_manager_;
    RadioactivityCapacityManager capacity_manager_;
    mfem::BilinearForm capacity_form_; // 固定网格、空间下 M 不变, 初始化时组装一次; 衰变矩阵为 lambda M
};

}
