#pragma once
#include "physics/thermal/common/thermal_domain_model.hpp"
namespace trimuse
{

class RhoCpCapacity final : public ThermalDomainModel
{
public:
    explicit RhoCpCapacity(const ModelParameterMap&) {}

    double evaluate(const PropertiesFieldData& properties, std::size_t point) const override
    {
        return scalar(properties, "material.general.mass_density", point) * scalar(properties, "material.thermal.specific_heat", point);
    }
};

}
