#pragma once

#include "properties/material/general/bacon_anisotropy_factor/bacon_anisotropy_factor_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantBaconAnisotropyFactor final : public BaconAnisotropyFactorModel, private ConstantModel
{
public:
    explicit ConstantBaconAnisotropyFactor(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
