#include "properties/material/mechanics/poisson_ratio/common/time_series_poisson_ratio.hpp"

namespace trimuse
{

TimeSeriesPoissonRatio::TimeSeriesPoissonRatio(const ModelParameterMap& parameters)
    : TimeSeriesModel(parameters)
{
}

double TimeSeriesPoissonRatio::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(TimeSeriesModel::evaluate(point));
}

}
