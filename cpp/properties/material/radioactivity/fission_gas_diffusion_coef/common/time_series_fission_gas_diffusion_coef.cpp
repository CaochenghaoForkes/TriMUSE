#include "properties/material/radioactivity/fission_gas_diffusion_coef/common/time_series_fission_gas_diffusion_coef.hpp"

namespace trimuse
{

TimeSeriesFissionGasDiffusionCoef::TimeSeriesFissionGasDiffusionCoef(const ModelParameterMap& parameters)
    :
    TimeSeriesModel(parameters)
{
}

double TimeSeriesFissionGasDiffusionCoef::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(TimeSeriesModel::evaluate(point));
}

}
