#pragma once

#include <cstddef>

#include "common/state_value.hpp"

namespace trimuse
{

class MechanicsIntegrator;

class MechanicsState // 保存求积点上的力学状态，与 MechanicsAssembly::State 中的矩阵和载荷区分
{
public:
    struct Values // 下标为全域求积点编号，六分量顺序为 [11,22,33,23,13,12]，采用求解坐标系和张量剪应变
    {
        std::vector<VectorValue> strain; // 总应变 epsilon = B d，不扣除本征应变
        std::vector<VectorValue> stress; // 应力 sigma = C epsilon - beta，beta = C epsilon*
        std::vector<VectorValue> principal_strain; // 三个主总应变，按降序排列
        std::vector<VectorValue> principal_stress; // 三个主应力，按降序排列
        std::vector<double> max_tensor_shear_strain; // 最大张量剪应变 (epsilon1 - epsilon3) / 2
    };

    explicit MechanicsState(std::size_t point_count); // 按全域求积点总数准备试探与已接受状态的存储

    void update(const MechanicsIntegrator& integrator, const std::vector<ElasticityTensorValue>& elasticity, const std::vector<VectorValue>& eigenstrain_coefficients); // 调用积分器，根据当前试探位移和传入的本构系数，更新各求积点的试探应变、应力及其主值和最大张量剪应变

    void load(); // 加载 试探应力、总应变及其主值和最大张量剪应变 <- 对应正式状态

    void accept(); // 接收 正式应力、总应变及其主值和最大张量剪应变 <- 对应试探状态

    const Values& trial_values() const; // 输出试探应力、总应变及其主值和最大张量剪应变

    const Values& accepted_values() const; // 输出正式应力、总应变及其主值和最大张量剪应变

private:
    Values trial_values_; // 试探状态包含 六分量总应变 六分量应力 及各自的三个主值和最大张量剪应变
    Values accepted_values_; // 已接受状态包含 六分量总应变 六分量应力 及各自的三个主值和最大张量剪应变
};

}
