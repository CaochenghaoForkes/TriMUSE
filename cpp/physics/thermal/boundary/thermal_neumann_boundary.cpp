#include "physics/thermal/boundary/thermal_neumann_boundary.hpp"

namespace trimuse
{

ThermalNeumannBoundary::ThermalNeumannBoundary(const ModelParameterMap&)
{
}

ThermalBoundaryValues ThermalNeumannBoundary::evaluate(const PropertiesFieldData& properties, int zone) const
{
    return {0.0, -scalar(properties, "external.heat_flux", zone), 0.0}; // 热流向外为正只添加到右端载荷向量
}

}
