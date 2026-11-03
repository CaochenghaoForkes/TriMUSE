#pragma once

#include "properties/material/radioactivity/nuclide_diffusion_coef/common/arrhenius_terms.hpp"
#include "properties/material/radioactivity/nuclide_diffusion_coef/nuclide_diffusion_coef_model_manager.hpp"

namespace trimuse
{

class TecDoc978USASiCNuclideDiffusionCoef final : public NuclideDiffusionCoefModel
{
public:
    explicit TecDoc978USASiCNuclideDiffusionCoef(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;

private:
    ArrheniusTerms terms_;
};

class TecDoc978USALowerSiCNuclideDiffusionCoef final : public NuclideDiffusionCoefModel
{
public:
    explicit TecDoc978USALowerSiCNuclideDiffusionCoef(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;
};

class TecDoc978USAUpperSiCNuclideDiffusionCoef final : public NuclideDiffusionCoefModel
{
public:
    explicit TecDoc978USAUpperSiCNuclideDiffusionCoef(const ModelParameterMap& parameters);
    
    double evaluate(const PropertiesStatePoint& state_point) const override;
};

}
