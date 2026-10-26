#pragma once

#include "properties/material/radioactivity/nuclide_diffusion_coef/nuclide_diffusion_coef_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantNuclideDiffusionCoef final : public NuclideDiffusionCoefModel, private ConstantModel
{
public:
    explicit ConstantNuclideDiffusionCoef(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
