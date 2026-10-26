#include "properties/external/power/time_series_power.hpp"

namespace trimuse
{

TimeSeriesPower::TimeSeriesPower(const ModelParameterMap& parameters, const std::string& value_type)
    : TimeSeriesModel(parameters, value_type)
{
}

StateValue TimeSeriesPower::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
