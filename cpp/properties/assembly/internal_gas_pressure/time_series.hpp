#pragma once

#include "properties/assembly/internal_gas_pressure/internal_gas_pressure_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesInternalGasPressure final : public InternalGasPressureModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesInternalGasPressure(const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& point) const override;
};

}
