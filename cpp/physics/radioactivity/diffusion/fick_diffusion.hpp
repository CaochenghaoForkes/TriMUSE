#pragma once
#include "physics/radioactivity/common/radioactivity_domain_model.hpp"

namespace trimuse
{

class FickDiffusion final : public RadioactivityDomainModel
{
public:
    explicit FickDiffusion(const ModelParameterMap&) {}

    double evaluate(const PropertiesFieldData& properties, std::size_t point) const override
    {
        return scalar(properties, "material.radioactivity.nuclide_diffusion_coef", point);
    }
};

}
