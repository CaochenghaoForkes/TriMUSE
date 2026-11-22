#include "properties/derived/irradiation_time/accumulate.hpp"

namespace trimuse
{

AccumulateIrradiationTime::AccumulateIrradiationTime(const ModelParameterMap& parameters)
    :
    initial_value_(std::get<double>(parameters.at("initial_value")))
{
}

StateValue AccumulateIrradiationTime::evaluate(const PropertiesStatePoint& point) const
{
    const double previous = point.previous_value ? std::get<double>(*point.previous_value) : initial_value_;
    return previous + point.scalar("external.time_step"); // 从已接受状态累积，耦合迭代不重复计时
}

}
