#pragma once

#include "properties/material/radioactivity/nuclide_diffusion_coef/nuclide_diffusion_coef_model_manager.hpp"

namespace trimuse
{

class TecDoc978JapanPyCNuclideDiffusionCoef final : public NuclideDiffusionCoefModel
{
public:
    explicit TecDoc978JapanPyCNuclideDiffusionCoef(const ModelParameterMap& parameters);
    
    double evaluate(const PropertiesStatePoint& state_point) const override;
};

}
