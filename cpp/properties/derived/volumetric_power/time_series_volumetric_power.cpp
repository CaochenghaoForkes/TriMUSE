#include "properties/derived/volumetric_power/time_series_volumetric_power.hpp"

namespace trimuse
{

TimeSeriesVolumetricPower::TimeSeriesVolumetricPower(const ModelParameterMap& parameters)
    : TimeSeriesModel(parameters)
{
}

StateValue TimeSeriesVolumetricPower::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
