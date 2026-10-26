#include "properties/external/environment_temperature/time_series_environment_temperature.hpp"

namespace trimuse
{

TimeSeriesEnvironmentTemperature::TimeSeriesEnvironmentTemperature(const ModelParameterMap& parameters, const std::string& value_type)
    : TimeSeriesModel(parameters, value_type)
{
}

StateValue TimeSeriesEnvironmentTemperature::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
