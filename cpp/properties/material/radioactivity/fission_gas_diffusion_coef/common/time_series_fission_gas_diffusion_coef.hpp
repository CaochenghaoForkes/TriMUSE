#pragma once

#include "properties/material/radioactivity/fission_gas_diffusion_coef/fission_gas_diffusion_coef_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesFissionGasDiffusionCoef final : public FissionGasDiffusionCoefModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesFissionGasDiffusionCoef(const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& point) const override;
};

}
