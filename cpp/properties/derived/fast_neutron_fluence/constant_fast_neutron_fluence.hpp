#pragma once

#include "properties/derived/fast_neutron_fluence/fast_neutron_fluence_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantFastNeutronFluence final : public FastNeutronFluenceModel, private ConstantModel
{
public:
    explicit ConstantFastNeutronFluence(const ModelParameterMap& parameters);
    
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
