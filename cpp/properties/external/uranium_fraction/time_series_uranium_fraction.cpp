#include "properties/external/uranium_fraction/time_series_uranium_fraction.hpp"

namespace trimuse
{

TimeSeriesUraniumFraction::TimeSeriesUraniumFraction(const ModelParameterMap& parameters, const std::string& value_type)
    : TimeSeriesModel(parameters, value_type)
{
}

StateValue TimeSeriesUraniumFraction::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
