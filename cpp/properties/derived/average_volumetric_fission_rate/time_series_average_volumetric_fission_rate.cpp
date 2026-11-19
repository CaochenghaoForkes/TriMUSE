#include "properties/derived/average_volumetric_fission_rate/time_series_average_volumetric_fission_rate.hpp"

namespace trimuse
{

TimeSeriesAverageVolumetricFissionRate::TimeSeriesAverageVolumetricFissionRate(const ModelParameterMap& parameters)
    :
    TimeSeriesModel(parameters)
{
}

StateValue TimeSeriesAverageVolumetricFissionRate::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
