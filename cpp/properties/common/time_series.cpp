#include "properties/common/time_series.hpp"
#include "properties/common/constant.hpp"
#include "common/linear_interpolation.hpp"

namespace trimuse
{

TimeSeriesModel::TimeSeriesModel(const ModelParameterMap& parameters, const std::string& value_type)
    : times_(std::get<VectorValue>(parameters.at("times")))
{
    const VectorValue& samples = std::get<VectorValue>(parameters.at("values"));
    const std::size_t components = value_type == "scalar" ? 1
        : value_type == "tensor2d" ? 4 : value_type == "tensor3d" ? 9 : samples.size() / times_.size();
    values_.reserve(times_.size());
    for (std::size_t index = 0; index < times_.size(); ++index)
    {
        const VectorValue sample(samples.begin() + index * components, samples.begin() + (index + 1) * components);
        values_.push_back(ConstantModel(sample, value_type).evaluate(PropertiesStatePoint{}));
    }
}

StateValue TimeSeriesModel::evaluate(const PropertiesStatePoint& point) const
{
    return LinearInterpolator::evaluate(times_, values_, point.scalar("time"));
}

}
