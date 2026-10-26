#pragma once

#include "properties/external/surface_temperature/surface_temperature_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesSurfaceTemperature final : public SurfaceTemperatureModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesSurfaceTemperature(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
