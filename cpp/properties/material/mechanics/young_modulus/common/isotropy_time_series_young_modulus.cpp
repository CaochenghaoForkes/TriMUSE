#include "properties/material/mechanics/young_modulus/common/isotropy_time_series_young_modulus.hpp"
#include "common/linear_interpolation.hpp"

namespace trimuse
{

IsotropyTimeSeriesYoungModulus::IsotropyTimeSeriesYoungModulus(const ModelParameterMap& parameters)
    : times_(std::get<VectorValue>(parameters.at("times"))), values_(std::get<VectorValue>(parameters.at("values")))
{
}

VectorValue IsotropyTimeSeriesYoungModulus::evaluate(const PropertiesStatePoint& point) const
{
    const double value = LinearInterpolator::evaluate(times_, values_, point.scalar("external.time"));
    return {value, value};
}

}
