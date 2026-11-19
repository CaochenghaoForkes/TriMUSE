#pragma once

#include "properties/derived/fission_gas_production_rate/fission_gas_production_rate_model_manager.hpp"

namespace trimuse
{

class Bison2021KernelFissionGasProductionRate final : public FissionGasProductionRateModel
{
public:
    explicit Bison2021KernelFissionGasProductionRate(const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
