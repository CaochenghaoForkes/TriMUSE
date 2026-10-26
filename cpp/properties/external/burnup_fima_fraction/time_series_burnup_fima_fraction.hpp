#pragma once

#include "properties/external/burnup_fima_fraction/burnup_fima_fraction_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesBurnupFimaFraction final : public BurnupFimaFractionModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesBurnupFimaFraction(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
