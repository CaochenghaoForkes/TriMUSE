#include "properties/external/surface_displacement/time_series_surface_displacement.hpp"

namespace trimuse
{

TimeSeriesSurfaceDisplacement::TimeSeriesSurfaceDisplacement(const ModelParameterMap& parameters)
    :
    TimeSeriesModel(parameters, "vector")
{
}

VectorValue TimeSeriesSurfaceDisplacement::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<VectorValue>(TimeSeriesModel::evaluate(point));
}

}
