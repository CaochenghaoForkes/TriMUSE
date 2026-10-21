#pragma once

#include "properties/material/radioactivity/nuclide_diffusion_coef/nuclide_diffusion_coef_model_manager.hpp"

namespace trimuse
{

class ArrheniusNuclideDiffusionCoef final : public NuclideDiffusionCoefModel
{
public:
    explicit ArrheniusNuclideDiffusionCoef(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;

private:
    double D0_; // m²/s
    double Q_; // J/mol
};

}
