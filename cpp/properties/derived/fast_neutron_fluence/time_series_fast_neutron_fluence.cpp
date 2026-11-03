#include "properties/derived/fast_neutron_fluence/time_series_fast_neutron_fluence.hpp"

namespace trimuse
{

TimeSeriesFastNeutronFluence::TimeSeriesFastNeutronFluence(const ModelParameterMap& parameters)
    : TimeSeriesModel(parameters)
{
}

StateValue TimeSeriesFastNeutronFluence::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
