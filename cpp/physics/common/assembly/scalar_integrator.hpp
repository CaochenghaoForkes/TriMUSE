#pragma once

#include "physics/common/field/trial_field.hpp"

namespace trimuse
{

// 标量 H1 场的域内和外边界积分。使用原网格几何与积分点编号，
// 通过 TrialField 取得连续场或分域场的形函数及完整自由度编号。
class ScalarIntegrator
{
public:
    ScalarIntegrator(FemSystem& fem, const TrialField& field) : fem_(fem), field_(field) {}

    std::shared_ptr<mfem::SparseMatrix> domain_matrix(int region, const std::vector<double>& values, bool diffusion);

    std::shared_ptr<mfem::Vector> domain_load(int region, const std::vector<double>& values);

    std::shared_ptr<mfem::SparseMatrix> boundary_matrix(const std::vector<int>& boundaries, double coefficient);

    std::shared_ptr<mfem::Vector> boundary_load(const std::vector<int>& boundaries, double coefficient);

private:
    FemSystem& fem_;
    
    const TrialField& field_;
};

}
