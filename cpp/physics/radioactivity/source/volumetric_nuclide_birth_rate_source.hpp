#pragma once
#include "physics/radioactivity/common/radioactivity_domain_model.hpp"

namespace trimuse
{

class VolumetricNuclideBirthRateSource final : public RadioactivityDomainModel // 读取体积核素产生率作为弱式右端系数
{
public:
    explicit VolumetricNuclideBirthRateSource(const ModelParameterMap&) {}

    double evaluate(const PropertiesFieldData& properties, std::size_t point) const override
    {
        return scalar(properties, "derived.volumetric_nuclide_birth_rate", point); // atom/(m³·s)
    }
};

}
