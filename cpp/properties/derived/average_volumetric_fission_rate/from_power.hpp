#pragma once

#include "properties/derived/average_volumetric_fission_rate/average_volumetric_fission_rate_model_manager.hpp"

namespace trimuse
{

class FromPower final : public AverageVolumetricFissionRateModel
{
public:
    explicit FromPower(const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
