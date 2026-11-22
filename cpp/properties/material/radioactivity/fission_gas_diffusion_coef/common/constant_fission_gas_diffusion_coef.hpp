#pragma once

#include "properties/material/radioactivity/fission_gas_diffusion_coef/fission_gas_diffusion_coef_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantFissionGasDiffusionCoef final : public FissionGasDiffusionCoefModel, private ConstantModel
{
public:
    explicit ConstantFissionGasDiffusionCoef(const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& point) const override;
};

}
