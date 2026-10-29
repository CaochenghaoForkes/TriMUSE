#pragma once
#include "physics/radioactivity/common/radioactivity_domain_model.hpp"

namespace trimuse
{

class VolumetricNuclideBirthRateSource final : public RadioactivityDomainModel
{
public:
    explicit VolumetricNuclideBirthRateSource(const ModelParameterMap&) {}

    double evaluate(const PropertiesFieldData& properties, std::size_t point) const override
    {
        return scalar(properties, "derived.volumetric_nuclide_birth_rate", point); // atom/(m³·s)
    }
};

}
