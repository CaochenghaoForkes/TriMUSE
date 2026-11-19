#include "physics/radioactivity/boundary/radioactivity_robin_boundary.hpp"

namespace trimuse
{

RadioactivityRobinBoundary::RadioactivityRobinBoundary(const ModelParameterMap&)
{
}

RadioactivityBoundaryValues RadioactivityRobinBoundary::evaluate(const PropertiesFieldData& properties, int zone) const
{
    const double h = scalar(properties, "derived.mass_transfer_coef", zone);
    // 左端 h_m c，右端 h_m c_infinity。
    return {h, h * scalar(properties, "external.environment_nuclide_concentration", zone), 0.0};
}

}
