#include "properties/material/mechanics/young_modulus/common/anisotropy_time_series_young_modulus.hpp"
#include "common/linear_interpolation.hpp"

namespace trimuse
{

AnisotropyTimeSeriesYoungModulus::AnisotropyTimeSeriesYoungModulus(const ModelParameterMap& parameters)
    : times_(std::get<VectorValue>(parameters.at("times"))),
      r_values_(std::get<VectorValue>(parameters.at("r_values"))),
      t_values_(std::get<VectorValue>(parameters.at("t_values")))
{
}

VectorValue AnisotropyTimeSeriesYoungModulus::evaluate(const PropertiesStatePoint& point) const
{
    const double time = point.scalar("external.time");
    return {LinearInterpolator::evaluate(times_, r_values_, time), LinearInterpolator::evaluate(times_, t_values_, time)};
}

}
