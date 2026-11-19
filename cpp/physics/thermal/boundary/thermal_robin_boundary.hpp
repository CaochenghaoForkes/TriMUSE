#pragma once
#include "physics/thermal/boundary/thermal_boundary_model.hpp"

namespace trimuse
{

class ThermalRobinBoundary final : public ThermalBoundaryModel
{
public:
    explicit ThermalRobinBoundary(const ModelParameterMap&);
    ThermalBoundaryValues evaluate(const PropertiesFieldData& properties, int zone) const override;
};

}
