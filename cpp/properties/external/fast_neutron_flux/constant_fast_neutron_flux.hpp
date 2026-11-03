#pragma once

#include "properties/external/fast_neutron_flux/fast_neutron_flux_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantFastNeutronFlux final : public FastNeutronFluxModel, private ConstantModel
{
public:
    explicit ConstantFastNeutronFlux(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
