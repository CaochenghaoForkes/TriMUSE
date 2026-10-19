#include "properties/material/general/porosity/common/time_series_porosity.hpp"

#include "common/linear_interpolation.hpp"

namespace trimuse
{

TimeSeriesPorosity::TimeSeriesPorosity(const ModelParameterMap& parameters)
    : times_(std::get<std::vector<double>>(parameters.at("times"))),
      values_(std::get<std::vector<double>>(parameters.at("values")))
{
}

double TimeSeriesPorosity::evaluate(const PropertiesStatePoint& state_point) const
{
    return LinearInterpolator::evaluate(times_, values_, state_point.scalar("time"));
}

}
