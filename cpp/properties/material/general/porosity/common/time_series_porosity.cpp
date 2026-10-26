#include "properties/material/general/porosity/common/time_series_porosity.hpp"

namespace trimuse
{

TimeSeriesPorosity::TimeSeriesPorosity(const ModelParameterMap& parameters)
    : TimeSeriesModel(parameters)
{
}

double TimeSeriesPorosity::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(TimeSeriesModel::evaluate(point));
}

}
