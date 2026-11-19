#pragma once

#include "properties/material/mechanics/thermal_expansion_coef/thermal_expansion_coef_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

// 各向同性常数，两个分量取相同值；返回 [alpha_r, alpha_t]，径向、切向热膨胀系数，单位 K^-1
class IsotropyConstantThermalExpansionCoef final : public ThermalExpansionCoefModel, private ConstantModel
{
public:
    explicit IsotropyConstantThermalExpansionCoef(const ModelParameterMap& parameters);
    VectorValue evaluate(const PropertiesStatePoint& point) const override;
};

}
