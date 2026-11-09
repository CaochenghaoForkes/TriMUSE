#pragma once

#include "properties/external/surface_traction/surface_traction_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesSurfaceTraction final : public SurfaceTractionModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesSurfaceTraction(const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesStatePoint& point) const override;
};

}
