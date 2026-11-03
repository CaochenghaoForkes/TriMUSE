#include "properties/derived/fast_neutron_fluence/accumulate_from_fast_neutron_flux.hpp"

namespace trimuse
{

AccumulateFromFastNeutronFlux::AccumulateFromFastNeutronFlux(const ModelParameterMap&)
{
}

StateValue AccumulateFromFastNeutronFlux::evaluate(const PropertiesStatePoint& point) const
{
    const double previous = point.previous_value ? std::get<double>(*point.previous_value) : 0.0;
    return previous + point.scalar("external.time_step") * point.scalar("external.fast_neutron_flux");
}

}
