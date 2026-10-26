#include "properties/material/radioactivity/nuclide_diffusion_coef/common/time_series_nuclide_diffusion_coef.hpp"

namespace trimuse
{

TimeSeriesNuclideDiffusionCoef::TimeSeriesNuclideDiffusionCoef(const ModelParameterMap& parameters)
    : TimeSeriesModel(parameters)
{
}

double TimeSeriesNuclideDiffusionCoef::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(TimeSeriesModel::evaluate(point));
}

}
