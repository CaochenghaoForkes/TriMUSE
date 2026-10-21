#pragma once

#include "properties/derived/volumetric_nuclide_birth_rate/volumetric_nuclide_birth_rate_model_manager.hpp"

namespace trimuse
{

class UniformFromBirthRate final : public VolumetricNuclideBirthRateModel
{
public:
    explicit UniformFromBirthRate(const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
