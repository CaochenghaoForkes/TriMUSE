#pragma once

#include "properties/derived/volumetric_nuclide_birth_rate/volumetric_nuclide_birth_rate_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantVolumetricNuclideBirthRate final : public VolumetricNuclideBirthRateModel, private ConstantModel
{
public:
    explicit ConstantVolumetricNuclideBirthRate(const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
