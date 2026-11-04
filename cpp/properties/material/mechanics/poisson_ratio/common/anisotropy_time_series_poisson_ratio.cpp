#include "properties/material/mechanics/poisson_ratio/common/anisotropy_time_series_poisson_ratio.hpp"
#include "common/linear_interpolation.hpp"

namespace trimuse
{

AnisotropyTimeSeriesPoissonRatio::AnisotropyTimeSeriesPoissonRatio(const ModelParameterMap& parameters)
    : times_(std::get<VectorValue>(parameters.at("times"))),
      rt_values_(std::get<VectorValue>(parameters.at("rt_values"))),
      tt_values_(std::get<VectorValue>(parameters.at("tt_values")))
{
}

VectorValue AnisotropyTimeSeriesPoissonRatio::evaluate(const PropertiesStatePoint& point) const
{
    const double time = point.scalar("external.time");
    return {LinearInterpolator::evaluate(times_, rt_values_, time), LinearInterpolator::evaluate(times_, tt_values_, time)};
}

}
