#pragma once

#include "properties/external/surface_pressure/surface_pressure_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesSurfacePressure final : public SurfacePressureModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesSurfacePressure(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
