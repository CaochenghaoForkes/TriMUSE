#pragma once

#include "properties/material/mechanics/shear_modulus/shear_modulus_model_manager.hpp"

namespace trimuse
{

// 各向同性时 Grt = E / (2*(1+nu))，单位 Pa。
// E、nu 已按统一接口存为双分量数组；各向同性时两个分量相同。
class IsotropyShearModulus final : public ShearModulusModel
{
public:
    explicit IsotropyShearModulus(const ModelParameterMap&) {}

    double evaluate(const PropertiesStatePoint& state_point) const override;
};

}
