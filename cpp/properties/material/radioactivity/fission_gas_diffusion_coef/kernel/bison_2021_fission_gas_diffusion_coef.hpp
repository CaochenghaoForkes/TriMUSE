#pragma once

#include "properties/material/radioactivity/fission_gas_diffusion_coef/fission_gas_diffusion_coef_model_manager.hpp"

namespace trimuse
{

class Bison2021FissionGasDiffusionCoef final : public FissionGasDiffusionCoefModel
{
public:
    explicit Bison2021FissionGasDiffusionCoef(const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& point) const override;
};

}
