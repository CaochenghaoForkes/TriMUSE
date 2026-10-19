#pragma once

#include "properties/derived/volumetric_power/volumetric_power_model_manager.hpp"

namespace trimuse
{

class UniformVolumetricPower final : public VolumetricPowerModel
{
public:
    explicit UniformVolumetricPower(const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
