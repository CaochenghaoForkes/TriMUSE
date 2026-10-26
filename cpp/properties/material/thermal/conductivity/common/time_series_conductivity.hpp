#pragma once

#include "properties/material/thermal/conductivity/conductivity_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesConductivity final : public ConductivityModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesConductivity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
