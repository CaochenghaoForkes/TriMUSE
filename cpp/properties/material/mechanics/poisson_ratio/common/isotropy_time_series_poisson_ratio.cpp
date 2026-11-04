#include "properties/material/mechanics/poisson_ratio/common/isotropy_time_series_poisson_ratio.hpp"
#include "common/linear_interpolation.hpp"

namespace trimuse
{

IsotropyTimeSeriesPoissonRatio::IsotropyTimeSeriesPoissonRatio(const ModelParameterMap& parameters)
    : times_(std::get<VectorValue>(parameters.at("times"))),
      values_(std::get<VectorValue>(parameters.at("values")))
{
}

VectorValue IsotropyTimeSeriesPoissonRatio::evaluate(const PropertiesStatePoint& point) const
{
    const double value = LinearInterpolator::evaluate(times_, values_, point.scalar("external.time"));
    return {value, value};
}

}
