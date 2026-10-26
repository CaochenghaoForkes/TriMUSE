#include "properties/material/thermal/conductivity/common/time_series_conductivity.hpp"

namespace trimuse
{

TimeSeriesConductivity::TimeSeriesConductivity(const ModelParameterMap& parameters)
    : TimeSeriesModel(parameters)
{
}

double TimeSeriesConductivity::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(TimeSeriesModel::evaluate(point));
}

}
