#include "properties/external/fast_neutron_flux/time_series_fast_neutron_flux.hpp"

namespace trimuse
{

TimeSeriesFastNeutronFlux::TimeSeriesFastNeutronFlux(const ModelParameterMap& parameters, const std::string& value_type)
    : TimeSeriesModel(parameters, value_type)
{
}

StateValue TimeSeriesFastNeutronFlux::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
