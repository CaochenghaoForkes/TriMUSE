#include "properties/derived/volumetric_nuclide_birth_rate/time_series_volumetric_nuclide_birth_rate.hpp"

namespace trimuse
{

TimeSeriesVolumetricNuclideBirthRate::TimeSeriesVolumetricNuclideBirthRate(const ModelParameterMap& parameters)
    : TimeSeriesModel(parameters)
{
}

StateValue TimeSeriesVolumetricNuclideBirthRate::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
