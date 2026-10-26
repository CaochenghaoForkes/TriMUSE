#include "properties/external/surface_temperature/time_series_surface_temperature.hpp"

namespace trimuse
{

TimeSeriesSurfaceTemperature::TimeSeriesSurfaceTemperature(const ModelParameterMap& parameters, const std::string& value_type)
    : TimeSeriesModel(parameters, value_type)
{
}

StateValue TimeSeriesSurfaceTemperature::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
