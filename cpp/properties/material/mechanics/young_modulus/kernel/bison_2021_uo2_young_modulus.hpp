#pragma once

#include "properties/material/mechanics/young_modulus/young_modulus_model_manager.hpp"

namespace trimuse
{

// UO2 杨氏模量，返回 [Er, Et]，单位 Pa；理论密度固定为纯 UO2 的值
class Bison2021Uo2YoungModulus final : public YoungModulusModel
{
public:
    explicit Bison2021Uo2YoungModulus(const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesStatePoint& state_point) const override;
};

}
