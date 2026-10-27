#pragma once
#include "physics/thermal/boundary/thermal_boundary_model.hpp"
namespace trimuse
{

class ThermalRobinBoundary final : public ThermalBoundaryModel
{
public:
    explicit ThermalRobinBoundary(const ModelParameterMap&) {}

    ThermalBoundaryValues evaluate(const PropertiesFieldData& properties, int zone) const override
    {
        const double h = scalar(properties, "derived.heat_transfer_coef", zone);
        return {h, h * scalar(properties, "external.environment_temperature", zone), 0.0};
    }
};

}
