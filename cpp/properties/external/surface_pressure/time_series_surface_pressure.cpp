#include "properties/external/surface_pressure/time_series_surface_pressure.hpp"

namespace trimuse
{

TimeSeriesSurfacePressure::TimeSeriesSurfacePressure(const ModelParameterMap& parameters)
    : TimeSeriesModel(parameters, "scalar")
{
}

double TimeSeriesSurfacePressure::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(TimeSeriesModel::evaluate(point));
}

}
