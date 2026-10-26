#include "properties/external/burnup_fima_fraction/time_series_burnup_fima_fraction.hpp"

namespace trimuse
{

TimeSeriesBurnupFimaFraction::TimeSeriesBurnupFimaFraction(const ModelParameterMap& parameters, const std::string& value_type)
    : TimeSeriesModel(parameters, value_type)
{
}

StateValue TimeSeriesBurnupFimaFraction::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
