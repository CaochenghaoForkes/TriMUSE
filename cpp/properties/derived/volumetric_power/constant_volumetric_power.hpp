#pragma once

#include "properties/derived/volumetric_power/volumetric_power_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantVolumetricPower final : public VolumetricPowerModel, private ConstantModel
{
public:
    explicit ConstantVolumetricPower(const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
