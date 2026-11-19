#pragma once

#include "properties/derived/fission_gas_production_rate/fission_gas_production_rate_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantFissionGasProductionRate final : public FissionGasProductionRateModel, private ConstantModel
{
public:
    explicit ConstantFissionGasProductionRate(const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
