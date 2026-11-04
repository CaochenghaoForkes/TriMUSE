#pragma once

#include "properties/material/mechanics/shear_modulus/shear_modulus_model_manager.hpp"

namespace trimuse
{

// 各向同性线弹性的第二拉梅常数（剪切模量）mu = E / (2*(1+nu)), 单位 Pa。
// 固定关系模型不需要额外构造参数，直接读取当前积分点的 E 和 nu。
class FixedRelationShearModulus final : public ShearModulusModel
{
public:
    explicit FixedRelationShearModulus(const ModelParameterMap&) {}

    double evaluate(const PropertiesStatePoint& state_point) const override;
};

}
