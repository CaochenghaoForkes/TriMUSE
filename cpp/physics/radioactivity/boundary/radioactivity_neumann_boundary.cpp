#include "physics/radioactivity/boundary/radioactivity_neumann_boundary.hpp"

namespace trimuse
{

RadioactivityNeumannBoundary::RadioactivityNeumannBoundary(const ModelParameterMap&)
{
}

RadioactivityBoundaryValues RadioactivityNeumannBoundary::evaluate(const PropertiesFieldData& properties, int zone) const
{
    // j_n = -D grad(c) · n, 外流为正; 弱形式右端系数为 -j_n。
    return {0.0, -scalar(properties, "external.nuclide_flux", zone), 0.0};
}

}
