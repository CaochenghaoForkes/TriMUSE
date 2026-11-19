#include "physics/radioactivity/boundary/radioactivity_dirichlet_boundary.hpp"

namespace trimuse
{

RadioactivityDirichletBoundary::RadioactivityDirichletBoundary(const ModelParameterMap&)
{
}

RadioactivityBoundaryValues RadioactivityDirichletBoundary::evaluate(const PropertiesFieldData& properties, int zone) const
{
    return {0.0, 0.0, scalar(properties, "external.surface_nuclide_concentration", zone)};
}

}
