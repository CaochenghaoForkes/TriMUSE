#pragma once

#include "properties/material/mechanics/young_modulus/young_modulus_model_manager.hpp"

namespace trimuse
{

// PyC 横观各向同性杨氏模量，直接返回 [Er, Et]，单位 Pa，不取平均
class Bison2021PycYoungModulus final : public YoungModulusModel
{
public:
    explicit Bison2021PycYoungModulus(const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesStatePoint& state_point) const override;
};

}
