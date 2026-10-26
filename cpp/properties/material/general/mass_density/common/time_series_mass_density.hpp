#pragma once

#include "properties/material/general/mass_density/mass_density_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesMassDensity final : public MassDensityModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesMassDensity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
