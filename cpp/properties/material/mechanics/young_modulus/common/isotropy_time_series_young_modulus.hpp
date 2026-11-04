#pragma once

#include "properties/material/mechanics/young_modulus/young_modulus_model_manager.hpp"

namespace trimuse
{

// 各向同性时间序列；返回 [Er, Et]，区间外保持端点值。
class IsotropyTimeSeriesYoungModulus final : public YoungModulusModel
{
public:
    explicit IsotropyTimeSeriesYoungModulus(const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesStatePoint& point) const override;

private:
    VectorValue times_;
    VectorValue values_;
};

}
