#pragma once

#include "properties/material/mechanics/thermal_expansion_coef/thermal_expansion_coef_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

// 横观各向同性常数；返回 [alpha_r, alpha_t]，径向、切向热膨胀系数，单位 K^-1
class AnisotropyConstantThermalExpansionCoef final : public ThermalExpansionCoefModel, private ConstantModel
{
public:
    explicit AnisotropyConstantThermalExpansionCoef(const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesStatePoint& point) const override;
};

}
