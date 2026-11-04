#pragma once

#include "properties/material/mechanics/poisson_ratio/poisson_ratio_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

// 各向同性常数，两个分量取相同值；返回 [nu_rt, nu_tt]，径向受力的切向泊松比、切向之间的泊松比。
class IsotropyConstantPoissonRatio final : public PoissonRatioModel, private ConstantModel
{
public:
    explicit IsotropyConstantPoissonRatio(const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesStatePoint& point) const override;
};

}
