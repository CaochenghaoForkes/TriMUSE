#pragma once

#include "properties/material/general/bacon_anisotropy_factor/bacon_anisotropy_factor_model_manager.hpp"

namespace trimuse
{

class Bison2021BaconAnisotropyFactor final : public BaconAnisotropyFactorModel
{
public:
    explicit Bison2021BaconAnisotropyFactor(const ModelParameterMap&);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
