#pragma once

#include "properties/assembly/internal_gas_pressure/internal_gas_pressure_model_manager.hpp"

namespace trimuse
{

class RkFromCoFissionGasRelease final : public InternalGasPressureModel
{
public:
    explicit RkFromCoFissionGasRelease(const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& point) const override;

private:
    int source_region_;
};

}
