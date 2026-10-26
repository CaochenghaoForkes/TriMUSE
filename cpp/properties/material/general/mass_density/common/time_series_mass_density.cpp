#include "properties/material/general/mass_density/common/time_series_mass_density.hpp"

namespace trimuse
{

TimeSeriesMassDensity::TimeSeriesMassDensity(const ModelParameterMap& parameters)
    : TimeSeriesModel(parameters)
{
}

double TimeSeriesMassDensity::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(TimeSeriesModel::evaluate(point));
}

}
