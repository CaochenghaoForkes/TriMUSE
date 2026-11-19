#pragma once

#include "properties/derived/average_volumetric_fission_rate/average_volumetric_fission_rate_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantAverageVolumetricFissionRate final : public AverageVolumetricFissionRateModel, private ConstantModel
{
public:
    explicit ConstantAverageVolumetricFissionRate(const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
