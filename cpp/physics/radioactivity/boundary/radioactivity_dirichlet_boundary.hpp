#pragma once
#include "physics/radioactivity/boundary/radioactivity_boundary_model.hpp"
namespace trimuse
{

class RadioactivityDirichletBoundary final : public RadioactivityBoundaryModel
{
public:
    explicit RadioactivityDirichletBoundary(const ModelParameterMap&) {}

    bool essential() const override { return true; }

    RadioactivityBoundaryValues evaluate(const PropertiesFieldData& properties, int zone) const override
    {
        return {0.0, 0.0, scalar(properties, "external.surface_nuclide_concentration", zone)};
    }
};

}
