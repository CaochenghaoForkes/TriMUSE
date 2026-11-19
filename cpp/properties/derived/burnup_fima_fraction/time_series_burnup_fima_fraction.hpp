#pragma once

#include "properties/derived/burnup_fima_fraction/burnup_fima_fraction_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesBurnupFimaFraction final : public BurnupFimaFractionModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesBurnupFimaFraction(const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
