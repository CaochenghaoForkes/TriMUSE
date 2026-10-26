#include "properties/external/nuclide_flux/time_series_nuclide_flux.hpp"

namespace trimuse
{

TimeSeriesNuclideFlux::TimeSeriesNuclideFlux(const ModelParameterMap& parameters, const std::string& value_type)
    : TimeSeriesModel(parameters, value_type)
{
}

StateValue TimeSeriesNuclideFlux::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
