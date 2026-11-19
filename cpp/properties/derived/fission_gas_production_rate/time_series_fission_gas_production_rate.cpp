#include "properties/derived/fission_gas_production_rate/time_series_fission_gas_production_rate.hpp"

namespace trimuse
{

TimeSeriesFissionGasProductionRate::TimeSeriesFissionGasProductionRate(const ModelParameterMap& parameters)
    :
    TimeSeriesModel(parameters)
{
}

StateValue TimeSeriesFissionGasProductionRate::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
