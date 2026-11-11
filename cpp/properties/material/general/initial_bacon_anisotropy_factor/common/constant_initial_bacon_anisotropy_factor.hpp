#pragma once

#include "properties/material/general/initial_bacon_anisotropy_factor/initial_bacon_anisotropy_factor_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantInitialBaconAnisotropyFactor final : public InitialBaconAnisotropyFactorModel, private ConstantModel
{
public:
    explicit ConstantInitialBaconAnisotropyFactor(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
