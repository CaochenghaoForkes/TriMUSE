#include "properties/external/surface_nuclide_concentration/time_series_surface_nuclide_concentration.hpp"

namespace trimuse
{

TimeSeriesSurfaceNuclideConcentration::TimeSeriesSurfaceNuclideConcentration(const ModelParameterMap& parameters, const std::string& value_type)
    : TimeSeriesModel(parameters, value_type)
{
}

StateValue TimeSeriesSurfaceNuclideConcentration::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
