#pragma once

#include "properties/external/heat_flux/heat_flux_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantHeatFlux final : public HeatFluxModel, private ConstantModel
{
public:
    explicit ConstantHeatFlux(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
