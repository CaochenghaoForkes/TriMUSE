#include "properties/material/general/bacon_anisotropy_factor/common/time_series_bacon_anisotropy_factor.hpp"

namespace trimuse
{

TimeSeriesBaconAnisotropyFactor::TimeSeriesBaconAnisotropyFactor(const ModelParameterMap& parameters)
    :
    TimeSeriesModel(parameters)
{
}

double TimeSeriesBaconAnisotropyFactor::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(TimeSeriesModel::evaluate(point));
}

}
