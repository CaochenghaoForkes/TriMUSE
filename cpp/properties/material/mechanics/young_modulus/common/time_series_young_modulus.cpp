#include "properties/material/mechanics/young_modulus/common/time_series_young_modulus.hpp"

namespace trimuse
{

TimeSeriesYoungModulus::TimeSeriesYoungModulus(const ModelParameterMap& parameters)
    : TimeSeriesModel(parameters)
{
}

double TimeSeriesYoungModulus::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(TimeSeriesModel::evaluate(point));
}

}
