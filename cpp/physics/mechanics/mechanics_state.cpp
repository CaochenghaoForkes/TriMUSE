#include "physics/mechanics/mechanics_state.hpp"
#include "physics/mechanics/assembly/mechanics_integrator.hpp"

#include <algorithm>
#include <cmath>

namespace trimuse
{

namespace
{

void principal_values(const VectorValue& values, VectorValue& principal) // 向量恢复为二阶张量并求特征值，从大到小排列存到 principal 中
{
    const double entries[3][3] = {
        {values[0], values[5], values[4]},
        {values[5], values[1], values[3]},
        {values[4], values[3], values[2]}}; // [11,22,33,23,13,12] 还原为对称张量，张量剪应变直接填入非对角位置
    const mfem::DenseMatrix tensor(entries);
    double directions[9]; // MFEM 接口同时输出主方向，此处仅使用主值
    tensor.CalcEigenvalues(principal.data(), directions); // principal.data()：接收三个特征值  directions：接收对应的三个特征向量
    std::reverse(principal.begin(), principal.end()); // MFEM 返回升序，统一改为从最大主值到最小主值
}

}

MechanicsState::MechanicsState(std::size_t point_count) // 传入 point_count 是原网格全部域单元的求积点总数
    :
    trial_values_{std::vector<VectorValue>(point_count, VectorValue(6, 0.0)), std::vector<VectorValue>(point_count, VectorValue(6, 0.0)), std::vector<VectorValue>(point_count, VectorValue(3, 0.0)), std::vector<VectorValue>(point_count, VectorValue(3, 0.0)), std::vector<double>(point_count, 0.0), std::vector<double>(point_count, 0.0)},
    accepted_values_(trial_values_)
{
}

void MechanicsState::update(const MechanicsIntegrator& integrator, const std::vector<ElasticityTensorValue>& elasticity, const std::vector<VectorValue>& eigenstrain_coefficients) // 调用积分器，根据当前试探位移和传入的本构系数，更新各求积点的试探应变、应力及其主值和 von Mises 应力
{
    integrator.recover_state(elasticity, eigenstrain_coefficients, trial_values_.strain, trial_values_.stress);
    for (std::size_t point = 0; point < trial_values_.strain.size(); ++point)
    {
        principal_values(trial_values_.strain[point], trial_values_.principal_strain[point]); // 主应变就是应变张量的特征值
        principal_values(trial_values_.stress[point], trial_values_.principal_stress[point]); // 主应力就是应力张量的特征值
        trial_values_.max_principal_stress[point] = trial_values_.principal_stress[point][0];
        const auto& stress = trial_values_.stress[point];
        const double difference12 = stress[0] - stress[1];
        const double difference23 = stress[1] - stress[2];
        const double difference31 = stress[2] - stress[0];
        trial_values_.von_mises_stress[point] = std::sqrt(0.5 * (difference12 * difference12 + difference23 * difference23 + difference31 * difference31) + 3.0 * (stress[3] * stress[3] + stress[4] * stress[4] + stress[5] * stress[5])); // sigma_vm = sqrt(3/2 s:s)，包含三个剪应力分量
    }
}

void MechanicsState::load()
{
    trial_values_ = accepted_values_;
}

void MechanicsState::accept()
{
    accepted_values_ = trial_values_;
}

const MechanicsState::Values& MechanicsState::trial_values() const
{
    return trial_values_;
}

const MechanicsState::Values& MechanicsState::accepted_values() const
{
    return accepted_values_;
}

}
