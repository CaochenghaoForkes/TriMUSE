#pragma once
#include "physics/thermal/boundary/thermal_boundary_model.hpp"
namespace trimuse
{

class ThermalNeumannBoundary final : public ThermalBoundaryModel
{
public:
    explicit ThermalNeumannBoundary(const ModelParameterMap&) {}

    ThermalBoundaryValues evaluate(const PropertiesFieldData& properties, int zone) const override
    {
        return {0.0, -scalar(properties, "external.heat_flux", zone), 0.0}; // 热流向外为正只添加到右端载荷向量
    }
};

}
