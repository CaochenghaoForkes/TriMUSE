#include "properties/material/general/mass_density/common/time_series_mass_density.hpp"

#include "common/linear_interpolation.hpp"

namespace trimuse
{

TimeSeriesMassDensity::TimeSeriesMassDensity(const ModelParameterMap& parameters)
    : times_(std::get<std::vector<double>>(parameters.at("times"))),
      values_(std::get<std::vector<double>>(parameters.at("values")))
{
}

double TimeSeriesMassDensity::evaluate(const PropertiesStatePoint& state_point) const
{
    return LinearInterpolator::evaluate(times_, values_, state_point.scalar("time"));
}

}
