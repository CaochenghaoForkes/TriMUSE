#pragma once

#include "properties/derived/average_volumetric_fission_rate/average_volumetric_fission_rate_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesAverageVolumetricFissionRate final : public AverageVolumetricFissionRateModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesAverageVolumetricFissionRate(const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
