#pragma once

#include "properties/derived/volumetric_power/volumetric_power_model_manager.hpp"

namespace trimuse
{

class UniformFromPower final : public VolumetricPowerModel
{
public:
    explicit UniformFromPower(const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
