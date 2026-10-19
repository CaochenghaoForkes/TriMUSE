#pragma once

#include <mfem.hpp>

namespace trimuse
{

struct LinearSolverConfig;

void solve_cg(const LinearSolverConfig& config, const mfem::Operator& matrix,
              const mfem::Vector& right_hand_side, mfem::Vector& solution);

}
