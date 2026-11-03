#pragma once

#include "properties/material/radioactivity/nuclide_diffusion_coef/common/arrhenius_terms.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/nuclide_diffusion_coef_model_manager.hpp"

namespace trimuse
{

class TecDoc978RussianPyCNuclideDiffusionCoef final : public NuclideDiffusionCoefModel
{
public:
    explicit TecDoc978RussianPyCNuclideDiffusionCoef(const ModelParameterMap& parameters);
    
    double evaluate(const PropertiesStatePoint& state_point) const override;

private:
    ArrheniusTerms terms_;
};

}
