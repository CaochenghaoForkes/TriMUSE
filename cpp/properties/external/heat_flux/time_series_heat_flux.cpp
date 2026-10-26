#include "properties/external/heat_flux/time_series_heat_flux.hpp"

namespace trimuse
{

TimeSeriesHeatFlux::TimeSeriesHeatFlux(const ModelParameterMap& parameters, const std::string& value_type)
    : TimeSeriesModel(parameters, value_type)
{
}

StateValue TimeSeriesHeatFlux::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
