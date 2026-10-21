#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <mfem.hpp>

namespace trimuse
{

class OutputManager;

struct LinearSolverConfig
{
    std::string method = "cg";
    std::string preconditioner = "none";
    double relative_tolerance = 1e-8;
    int max_iterations = 1000;
};

struct NonlinearSolverConfig
{
    std::string method = "picard";
    double relative_tolerance = 1e-8;
    double absolute_tolerance = 1e-8;
    int max_iterations = 50;
};

struct FieldSolverConfig
{
    std::string name;
    LinearSolverConfig linear;
    NonlinearSolverConfig nonlinear;
};

class SolverSystem
{
public:
    explicit SolverSystem(const std::vector<FieldSolverConfig>& configs);

    const NonlinearSolverConfig& nonlinear_config(const std::string& field_name) const;

    void solve_linear(const std::string& field_name, const mfem::Operator& matrix, const mfem::Vector& right_hand_side, mfem::Vector& solution, OutputManager& output_manager) const;

private:
    std::unordered_map<std::string, FieldSolverConfig> field_configs_;
};

}
