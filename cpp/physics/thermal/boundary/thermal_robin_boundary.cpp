#include "physics/thermal/boundary/thermal_robin_boundary.hpp"

namespace trimuse
{

ThermalRobinBoundary::ThermalRobinBoundary(const ModelParameterMap&)
{
}

ThermalBoundaryValues ThermalRobinBoundary::evaluate(const PropertiesFieldData& properties, int zone) const
{
    const double h = scalar(properties, "derived.heat_transfer_coef", zone);
    return {h, h * scalar(properties, "external.environment_temperature", zone), 0.0};
}

}
