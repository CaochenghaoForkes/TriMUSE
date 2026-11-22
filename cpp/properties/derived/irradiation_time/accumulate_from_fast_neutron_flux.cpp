#include "properties/derived/irradiation_time/accumulate_from_fast_neutron_flux.hpp"

namespace trimuse
{

AccumulateIrradiationTimeFromFastNeutronFlux::AccumulateIrradiationTimeFromFastNeutronFlux(const ModelParameterMap& parameters)
    :
    initial_value_(std::get<double>(parameters.at("initial_value")))
{
}

StateValue AccumulateIrradiationTimeFromFastNeutronFlux::evaluate(const PropertiesStatePoint& point) const
{
    const double previous = point.previous_value ? std::get<double>(*point.previous_value) : initial_value_;
    if (point.scalar("external.fast_neutron_flux") <= 0.0)
    {
        return previous;
    }
    return previous + point.scalar("external.time_step"); // 从已接受状态累积，耦合迭代不重复计时
}

}
