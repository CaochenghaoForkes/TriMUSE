#pragma once

#include "physics/common/field/trial_field.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

// 标量 H1 场的域内和外边界积分。使用原网格几何与积分点编号，
// 通过 TrialField 取得连续场或分域场的形函数及完整自由度编号。
class ScalarIntegrator // 接收已算好的系数和指定区域，负责积分并按完整场的自由度编号组装；它不负责决定导热系数、热源等属性怎么计算
{
public:
    enum class MatrixForm
    {
        Mass,
        Gradient
    }; // Mass: ∫ a Ni Nj dΩ，Gradient: ∫ a ∇Ni·∇Nj dΩ

    ScalarIntegrator(FemSystem& fem, const TrialField& trial_field);
    std::shared_ptr<mfem::SparseMatrix> domain_matrix(int region, const std::vector<double>& values, MatrixForm form, const PropertiesFieldData::DomainIntegrationPointValues& weights); // 按指定积分形式组装区域矩阵
    std::shared_ptr<mfem::Vector> domain_load(int region, const std::vector<double>& values, const PropertiesFieldData::DomainIntegrationPointValues& weights); // 整体向量组装（域内积分）：组装指定 region 的域内总体向量贡献，对应弱形式右端 F_i += ∫_{Ω_region} f N_i dΩ  region : 当前要组装的区域编号 values: 已经计算好的全域积分点系数数组，数组长度是全域积分点总数
    std::shared_ptr<mfem::SparseMatrix> boundary_matrix(const std::vector<int>& boundaries, double coefficient, const PropertiesFieldData::FaceIntegrationPointValues& weights); // 整体矩阵组装（边界积分）：组装指定外边界分区 zone 的矩阵贡献 当前主要对应 Robin 传递边界的左端项 int_gamma_zone h Ni Nj dGamma boundaries 传入的是该外边界分区所包含的原网格边界单元编号列表
    std::shared_ptr<mfem::Vector> boundary_load(const std::vector<int>& boundaries, double coefficient, const PropertiesFieldData::FaceIntegrationPointValues& weights); // 整体向量组装（边界积分）：组装指定外边界分区 zone 的右端向量贡献 int_gamma_zone value Ni dGamma

private:
    FemSystem& fem_; // 有限元系统
    const TrialField& trial_field_; // 试探解
};

}
