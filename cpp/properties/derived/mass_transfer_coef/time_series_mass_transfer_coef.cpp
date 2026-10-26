#include "properties/derived/mass_transfer_coef/time_series_mass_transfer_coef.hpp"

namespace trimuse
{

TimeSeriesMassTransferCoef::TimeSeriesMassTransferCoef(const ModelParameterMap& parameters)
    : TimeSeriesModel(parameters)
{
}

StateValue TimeSeriesMassTransferCoef::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
