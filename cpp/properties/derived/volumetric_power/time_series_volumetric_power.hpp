#pragma once

#include "properties/derived/volumetric_power/volumetric_power_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesVolumetricPower final : public VolumetricPowerModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesVolumetricPower(const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
