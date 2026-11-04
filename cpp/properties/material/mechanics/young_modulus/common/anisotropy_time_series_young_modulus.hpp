#pragma once

#include "properties/material/mechanics/young_modulus/young_modulus_model_manager.hpp"

namespace trimuse
{

// 横观各向同性时间序列；返回 [Er, Et]，区间外保持端点值。
class AnisotropyTimeSeriesYoungModulus final : public YoungModulusModel
{
public:
    explicit AnisotropyTimeSeriesYoungModulus(const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesStatePoint& point) const override;

private:
    VectorValue times_;
    VectorValue r_values_;
    VectorValue t_values_;
};

}
