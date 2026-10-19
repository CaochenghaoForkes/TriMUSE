#include "properties/external/common/time_series.hpp"
#include "common/linear_interpolation.hpp"

namespace trimuse
{

TimeSeriesExternalState::TimeSeriesExternalState(const std::vector<double>& times, const std::vector<StateValue>& values)
    : times_(times), values_(values)
{
}

StateValue TimeSeriesExternalState::evaluate(const PropertiesStatePoint& point) const
{
    return LinearInterpolator::evaluate(times_, values_, point.scalar("time"));
}

}
