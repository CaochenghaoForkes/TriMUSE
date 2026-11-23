#include "properties/assembly/cumulative_co_release/from_homogenized_panama_opf.hpp"
#include "properties/assembly/local_panama_opf/panama.hpp"

namespace trimuse
{

FromHomogenizedPanamaOpf::FromHomogenizedPanamaOpf(const ModelParameterMap&)
{
}

double FromHomogenizedPanamaOpf::evaluate(const PropertiesStatePoint& point) const
{
    const double opf = point.vector("assembly.homogenized_panama_opf")[LocalPanamaOpfState::opf];
    return point.scalar("external.initial_heavy_metal_inventory") * point.scalar("derived.burnup_fima_fraction") * opf; // mol，进入颗粒内部自由空间的 CO 累计量
}

}
