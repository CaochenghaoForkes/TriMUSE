#pragma once

#include "properties/material/mechanics/thermal_expansion_coef/thermal_expansion_coef_model_manager.hpp"

namespace trimuse
{

// Buffer 各向同性热膨胀系数，返回 [alpha_r, alpha_t]，单位 K^-1
class Bison2021BufferThermalExpansionCoef final : public ThermalExpansionCoefModel
{
public:
    explicit Bison2021BufferThermalExpansionCoef(const ModelParameterMap& parameters);
    VectorValue evaluate(const PropertiesStatePoint& state_point) const override;
};

}
