#include "properties/common/time_series.hpp"
#include "properties/common/constant.hpp"
#include "common/linear_interpolation.hpp"

namespace trimuse
{

TimeSeriesModel::TimeSeriesModel(const ModelParameterMap& parameters, const std::string& value_type)
    : times_(std::get<VectorValue>(parameters.at("times")))
{
    const VectorValue& samples = std::get<VectorValue>(parameters.at("values"));
    const std::size_t components = value_type == "scalar" ? 1 : value_type == "tensor2d" ? 4 : value_type == "tensor3d" ? 9 : samples.size() / times_.size(); // 如果已经指定了标量/2维张量/3维张量则可以直接判断出分量数,但 vector 类型只能根据全部样本数据量和时间点数量，推算每个时间点有几个分量
    values_.reserve(times_.size());
    for (std::size_t index = 0; index < times_.size(); ++index) // 遍历每个时间点
    {
        const VectorValue sample(samples.begin() + index * components, samples.begin() + (index + 1) * components); // 用两个迭代器构造数组，复制左闭右开区间内的数据 即为截取当前时间点的全部分量
        values_.push_back(ConstantModel(sample, value_type).evaluate(PropertiesStatePoint{})); // 创建一个临时常数模型，复用它的转换逻辑，将 sample 按 value_type 整理成 StateValue 标量/向量/2d张量/3d张量
    }
}

StateValue TimeSeriesModel::evaluate(const PropertiesStatePoint& point) const
{
    return LinearInterpolator::evaluate(times_, values_, point.scalar("external.time"));
}

}
