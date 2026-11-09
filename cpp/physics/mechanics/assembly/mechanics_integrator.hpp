#pragma once

#include "physics/common/field/trial_field.hpp"
#include "physics/mechanics/boundary/mechanics_boundary_model.hpp"

namespace trimuse
{

class MechanicsIntegrator // 按原网格积分，通过试探场的完整自由度编号组装力学矩阵和载荷
{
public:
    MechanicsIntegrator(FemSystem& fem, const TrialField& trial_field); // 初始化并缓存所有域内求积点的应变矩阵 B

    std::shared_ptr<mfem::SparseMatrix> domain_matrix(int region, const std::vector<ElasticityTensorValue>& values); // 组装 region 贡献的总体刚度矩阵 int_Omega (B^e)^T W C_{global} B^e

    std::shared_ptr<mfem::Vector> eigenstrain_load(int region, const std::vector<VectorValue>& values); // 组装 region 贡献的总体本征应变构成的载荷向量 int_Omega (B^e)^T W C_{global} \epsilon^*_{global}

    std::shared_ptr<mfem::Vector> boundary_load(const std::vector<int>& boundaries, const MechanicsBoundaryValue& value); // 组装各 boundaries(当前外边界分区包含的原网格边界单元编号) 单元由于指定面力(vector)或指定压力(double) 贡献的载荷向量

private:
    FemSystem& fem_;
    const TrialField& trial_field_;
    int dimension_;
    std::vector<mfem::DenseMatrix> strain_matrices_; // 全域求积点编号 -> 该点的应变矩阵 B，网格与位移空间固定时只需建立一次
};

}
