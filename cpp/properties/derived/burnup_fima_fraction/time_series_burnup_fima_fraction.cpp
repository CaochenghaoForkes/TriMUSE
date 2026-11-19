#include "properties/derived/burnup_fima_fraction/time_series_burnup_fima_fraction.hpp"

namespace trimuse
{

TimeSeriesBurnupFimaFraction::TimeSeriesBurnupFimaFraction(const ModelParameterMap& parameters)
    :
    TimeSeriesModel(parameters)
{
}

StateValue TimeSeriesBurnupFimaFraction::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
