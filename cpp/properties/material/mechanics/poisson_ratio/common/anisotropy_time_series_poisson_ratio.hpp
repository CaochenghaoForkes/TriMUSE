#pragma once

#include "properties/material/mechanics/poisson_ratio/poisson_ratio_model_manager.hpp"

namespace trimuse
{

// 横观各向同性时间序列；返回 [nu_rt, nu_tt]，区间外保持端点值。
class AnisotropyTimeSeriesPoissonRatio final : public PoissonRatioModel
{
public:
    explicit AnisotropyTimeSeriesPoissonRatio(const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesStatePoint& point) const override;

private:
    VectorValue times_;
    VectorValue rt_values_;
    VectorValue tt_values_;
};

}
