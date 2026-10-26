#include "properties/external/nuclide_birth_rate/time_series_nuclide_birth_rate.hpp"

namespace trimuse
{

TimeSeriesNuclideBirthRate::TimeSeriesNuclideBirthRate(const ModelParameterMap& parameters, const std::string& value_type)
    : TimeSeriesModel(parameters, value_type)
{
}

StateValue TimeSeriesNuclideBirthRate::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
