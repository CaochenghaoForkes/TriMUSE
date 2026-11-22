#pragma once

#include "properties/material/radioactivity/fission_gas_diffusion_coef/fission_gas_diffusion_coef_model_manager.hpp"

namespace trimuse
{

class ArrheniusFissionGasDiffusionCoef final : public FissionGasDiffusionCoefModel
{
public:
    explicit ArrheniusFissionGasDiffusionCoef(const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& state_point) const override;

private:
    double D0_; // m²/s
    double Q_; // J/mol
};

}
