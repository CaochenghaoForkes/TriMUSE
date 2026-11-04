#pragma once

#include "properties/material/mechanics/shear_modulus/shear_modulus_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantShearModulus final : public ShearModulusModel, private ConstantModel
{
public:
    explicit ConstantShearModulus(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
