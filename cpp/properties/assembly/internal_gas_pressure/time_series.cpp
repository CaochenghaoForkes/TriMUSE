#include "properties/assembly/internal_gas_pressure/time_series.hpp"

namespace trimuse
{

TimeSeriesInternalGasPressure::TimeSeriesInternalGasPressure(const ModelParameterMap& parameters)
    :
    TimeSeriesModel(parameters)
{
}

double TimeSeriesInternalGasPressure::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(TimeSeriesModel::evaluate(point));
}

}
