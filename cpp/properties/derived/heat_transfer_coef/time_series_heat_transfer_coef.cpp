#include "properties/derived/heat_transfer_coef/time_series_heat_transfer_coef.hpp"

namespace trimuse
{

TimeSeriesHeatTransferCoef::TimeSeriesHeatTransferCoef(const ModelParameterMap& parameters)
    : TimeSeriesModel(parameters)
{
}

StateValue TimeSeriesHeatTransferCoef::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
