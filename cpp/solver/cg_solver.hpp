#pragma once

#include <string>

#include <mfem.hpp>

namespace trimuse
{

struct LinearSolverConfig;
class OutputManager;

void solve_cg(const LinearSolverConfig& config, const mfem::Operator& matrix,
              const mfem::Vector& right_hand_side, mfem::Vector& solution,
              const std::string& field_name, OutputManager& output_manager);

}
