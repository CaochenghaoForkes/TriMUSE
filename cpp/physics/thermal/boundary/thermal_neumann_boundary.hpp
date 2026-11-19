#pragma once
#include "physics/thermal/boundary/thermal_boundary_model.hpp"

namespace trimuse
{

class ThermalNeumannBoundary final : public ThermalBoundaryModel
{
public:
    explicit ThermalNeumannBoundary(const ModelParameterMap&);
    ThermalBoundaryValues evaluate(const PropertiesFieldData& properties, int zone) const override;
};

}
