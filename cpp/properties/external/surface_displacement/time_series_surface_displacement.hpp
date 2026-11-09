#pragma once

#include "properties/external/surface_displacement/surface_displacement_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesSurfaceDisplacement final : public SurfaceDisplacementModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesSurfaceDisplacement(const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesStatePoint& point) const override;
};

}
