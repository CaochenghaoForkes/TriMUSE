#pragma once

#include "properties/external/environment_temperature/environment_temperature_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesEnvironmentTemperature final : public EnvironmentTemperatureModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesEnvironmentTemperature(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
