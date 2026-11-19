#pragma once

#include "properties/derived/fission_gas_production_rate/fission_gas_production_rate_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesFissionGasProductionRate final : public FissionGasProductionRateModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesFissionGasProductionRate(const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
