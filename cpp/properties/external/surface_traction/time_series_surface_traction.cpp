#include "properties/external/surface_traction/time_series_surface_traction.hpp"

namespace trimuse
{

TimeSeriesSurfaceTraction::TimeSeriesSurfaceTraction(const ModelParameterMap& parameters)
    : TimeSeriesModel(parameters, "vector")
{
}

VectorValue TimeSeriesSurfaceTraction::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<VectorValue>(TimeSeriesModel::evaluate(point));
}

}
