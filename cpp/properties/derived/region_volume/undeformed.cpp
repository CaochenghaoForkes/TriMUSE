#include "properties/derived/region_volume/undeformed.hpp"

namespace trimuse
{

Undeformed::Undeformed(const ModelParameterMap&)
{
}

StateValue Undeformed::evaluate(const PropertiesStatePoint& point) const
{
    return point.scalar("precomputed.initial_region_volume");
}

}
