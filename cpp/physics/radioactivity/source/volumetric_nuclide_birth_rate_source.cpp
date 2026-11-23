#include "physics/radioactivity/source/volumetric_nuclide_birth_rate_source.hpp"

namespace trimuse
{

VolumetricNuclideBirthRateSource::VolumetricNuclideBirthRateSource(const ModelParameterMap&)
{
}

double VolumetricNuclideBirthRateSource::evaluate(const PropertiesFieldData& properties, std::size_t point) const
{
    return scalar(properties, "derived.volumetric_nuclide_birth_rate", point); // mol/(m³·s)
}

}
