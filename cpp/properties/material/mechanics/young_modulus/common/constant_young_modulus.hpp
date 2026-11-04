#pragma once

#include "properties/material/mechanics/young_modulus/young_modulus_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantYoungModulus final : public YoungModulusModel, private ConstantModel
{
public:
    explicit ConstantYoungModulus(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
