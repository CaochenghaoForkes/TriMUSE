#pragma once

#include "properties/material/mechanics/lame_lambda/lame_lambda_model_manager.hpp"

namespace trimuse
{

// 各向同性线弹性的第一拉梅常数 lambda = E*nu / ((1+nu)*(1-2*nu)), 单位 Pa。
// 固定关系模型不需要额外构造参数，直接读取当前积分点的 E 和 nu。
class FixedRelationLameLambda final : public LameLambdaModel
{
public:
    explicit FixedRelationLameLambda(const ModelParameterMap&) {}

    double evaluate(const PropertiesStatePoint& state_point) const override;
};

}
