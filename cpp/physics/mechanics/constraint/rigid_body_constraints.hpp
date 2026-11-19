#pragma once

#include <map>

#include "physics/common/constraint/dof_constraints.hpp"

namespace trimuse
{

// 消除刚体移动：1D 补中心条件，2D 补轴线条件和轴向定位，3D 只补足尚未消除的刚体模态
void add_rigid_body_constraints(const FemSystem& fem, const TrialField& trial_field, const DofConstraints& constraints, std::map<int, double>& essential);

}
