#include "properties/material/thermal/specific_heat/common/time_series_specific_heat.hpp"

namespace trimuse
{

TimeSeriesSpecificHeat::TimeSeriesSpecificHeat(const ModelParameterMap& parameters)
    : TimeSeriesModel(parameters)
{
}

double TimeSeriesSpecificHeat::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(TimeSeriesModel::evaluate(point));
}

}
