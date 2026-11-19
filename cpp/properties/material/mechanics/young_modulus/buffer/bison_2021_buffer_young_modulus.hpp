#pragma once

#include "properties/material/mechanics/young_modulus/young_modulus_model_manager.hpp"

namespace trimuse
{

// Buffer 各向同性杨氏模量，返回 [Er, Et]，单位 Pa
class Bison2021BufferYoungModulus final : public YoungModulusModel
{
public:
    explicit Bison2021BufferYoungModulus(const ModelParameterMap& parameters);
    VectorValue evaluate(const PropertiesStatePoint& state_point) const override;
};

}
