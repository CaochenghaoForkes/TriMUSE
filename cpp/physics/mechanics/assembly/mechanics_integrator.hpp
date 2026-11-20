#pragma once

#include "physics/common/field/trial_field.hpp"
#include "properties/properties_field_data.hpp"
#include "physics/mechanics/boundary/mechanics_boundary_model.hpp"

namespace trimuse
{

class MechanicsIntegrator // 按原网格积分，通过试探场的完整自由度编号组装力学矩阵和载荷
{
public:
    MechanicsIntegrator(FemSystem& fem, const TrialField& trial_field); // 初始化并缓存所有域内求积点的应变矩阵 B
    std::shared_ptr<mfem::SparseMatrix> domain_matrix(int region, const std::vector<ElasticityTensorValue>& values, const PropertiesFieldData::IntegrationPointValues& weights); // 组装 region 贡献的总体刚度矩阵 int_Omega (B^e)^T W C_{global} B^e
    std::shared_ptr<mfem::Vector> domain_load(int region, const std::vector<VectorValue>& values, const PropertiesFieldData::IntegrationPointValues& weights); // values 为 manager 已算好的全域求积点体积力向量，组装 region 贡献的总体载荷 int_Omega (N^e)^T b
    std::shared_ptr<mfem::Vector> eigenstrain_load(int region, const std::vector<VectorValue>& values, const PropertiesFieldData::IntegrationPointValues& weights); // 组装 region 贡献的总体本征应变构成的载荷向量 int_Omega (B^e)^T W C_{global} \epsilon^*_{global}
    std::shared_ptr<mfem::Vector> boundary_load(const std::vector<int>& boundaries, const MechanicsBoundaryValue& value); // 组装各 boundaries(当前外边界分区包含的原网格边界单元编号) 单元由于指定面力(vector)或指定压力(double) 贡献的载荷向量
    void recover_state(const std::vector<ElasticityTensorValue>& elasticity, const std::vector<VectorValue>& eigenstrain_coefficients, std::vector<VectorValue>& strain_values, std::vector<VectorValue>& stress_values) const; // 从当前完整试探位移中提取各单元的位移，在每个求积点计算应变、应力，并直接写入传入的结果数组 传入弹性矩阵 CV 各求积点点 beta=Cv\epsilonV*

private:
    FemSystem& fem_;
    const TrialField& trial_field_;
    int dimension_;
    std::vector<mfem::DenseMatrix> strain_matrices_; // 全域求积点编号 -> 该点的应变矩阵 B，网格与位移空间固定时只需建立一次
};

}
