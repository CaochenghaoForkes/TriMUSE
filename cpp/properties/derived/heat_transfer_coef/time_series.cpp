#include "properties/derived/heat_transfer_coef/time_series.hpp"

#include "common/linear_interpolation.hpp"

namespace trimuse
{

TimeSeriesHeatTransferCoef::TimeSeriesHeatTransferCoef(const ModelParameterMap& parameters)
    : times_(std::get<std::vector<double>>(parameters.at("times"))),
      values_(std::get<std::vector<double>>(parameters.at("values")))
{
}

StateValue TimeSeriesHeatTransferCoef::evaluate(const PropertiesStatePoint& point) const
{
    return LinearInterpolator::evaluate(times_, values_, point.scalar("time"));
}

}
