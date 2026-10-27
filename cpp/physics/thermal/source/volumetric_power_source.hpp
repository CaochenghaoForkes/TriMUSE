#pragma once
#include "physics/thermal/common/thermal_domain_model.hpp"
namespace trimuse
{

class VolumetricPowerSource final : public ThermalDomainModel
{
public:
    explicit VolumetricPowerSource(const ModelParameterMap&) {}
    
    double evaluate(const PropertiesFieldData& properties, std::size_t point) const override
    {
        return scalar(properties, "derived.volumetric_power", point);
    }
};

}
