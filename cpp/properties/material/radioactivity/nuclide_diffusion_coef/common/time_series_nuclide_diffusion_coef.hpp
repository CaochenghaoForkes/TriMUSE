#pragma once

#include "properties/material/radioactivity/nuclide_diffusion_coef/nuclide_diffusion_coef_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesNuclideDiffusionCoef final : public NuclideDiffusionCoefModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesNuclideDiffusionCoef(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
