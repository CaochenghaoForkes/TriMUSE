#pragma once

#include "properties/material/mechanics/young_modulus/young_modulus_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

// 横观各向同性常数；返回 [Er, Et]，径向、切向杨氏模量，单位 Pa。
class AnisotropyConstantYoungModulus final : public YoungModulusModel, private ConstantModel
{
public:
    explicit AnisotropyConstantYoungModulus(const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesStatePoint& point) const override;
};

}
