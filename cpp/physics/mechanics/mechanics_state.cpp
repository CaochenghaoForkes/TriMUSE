#include "physics/mechanics/mechanics_state.hpp"
#include "physics/mechanics/assembly/mechanics_integrator.hpp"

namespace trimuse
{

MechanicsState::MechanicsState(std::size_t point_count) // 传入 point_count 是原网格全部域单元的求积点总数
    : trial_values_{std::vector<VectorValue>(point_count, VectorValue(6, 0.0)), std::vector<VectorValue>(point_count, VectorValue(6, 0.0))}, accepted_values_(trial_values_)
{
}

void MechanicsState::update(const MechanicsIntegrator& integrator, const std::vector<ElasticityTensorValue>& elasticity, const std::vector<VectorValue>& eigenstrain_coefficients) // 调用积分器，根据当前试探位移和传入的本构系数，更新各求积点的试探应变、应力
{
    integrator.recover_state(elasticity, eigenstrain_coefficients, trial_values_.strain, trial_values_.stress);
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
