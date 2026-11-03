#pragma once

#include "properties/derived/fast_neutron_fluence/fast_neutron_fluence_model_manager.hpp"

namespace trimuse
{

class AccumulateFromFastNeutronFlux final : public FastNeutronFluenceModel
{
public:
    explicit AccumulateFromFastNeutronFlux(const ModelParameterMap&);
    
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
