#pragma once

#include "properties/assembly/internal_gas_pressure/internal_gas_pressure_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantInternalGasPressure final : public InternalGasPressureModel, private ConstantModel
{
public:
    explicit ConstantInternalGasPressure(const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& point) const override;
};

}
