#include "properties/material/mechanics/shear_modulus/common/time_series_shear_modulus.hpp"

namespace trimuse
{

TimeSeriesShearModulus::TimeSeriesShearModulus(const ModelParameterMap& parameters)
    : TimeSeriesModel(parameters)
{
}

double TimeSeriesShearModulus::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(TimeSeriesModel::evaluate(point));
}

}
