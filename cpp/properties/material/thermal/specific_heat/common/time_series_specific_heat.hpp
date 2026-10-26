#pragma once

#include "properties/material/thermal/specific_heat/specific_heat_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesSpecificHeat final : public SpecificHeatModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesSpecificHeat(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
