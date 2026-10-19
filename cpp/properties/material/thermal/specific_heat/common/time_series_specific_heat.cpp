#include "properties/material/thermal/specific_heat/common/time_series_specific_heat.hpp"

#include "common/linear_interpolation.hpp"

namespace trimuse
{

TimeSeriesSpecificHeat::TimeSeriesSpecificHeat(const ModelParameterMap& parameters)
    : times_(std::get<std::vector<double>>(parameters.at("times"))),
      values_(std::get<std::vector<double>>(parameters.at("values")))
{
}

double TimeSeriesSpecificHeat::evaluate(const PropertiesStatePoint& state_point) const
{
    return LinearInterpolator::evaluate(times_, values_, state_point.scalar("time"));
}

}
