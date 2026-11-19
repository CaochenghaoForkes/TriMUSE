#include "physics/thermal/source/volumetric_power_source.hpp"

namespace trimuse
{

VolumetricPowerSource::VolumetricPowerSource(const ModelParameterMap&)
{
}

double VolumetricPowerSource::evaluate(const PropertiesFieldData& properties, std::size_t point) const
{
    return scalar(properties, "derived.volumetric_power", point);
}

}
