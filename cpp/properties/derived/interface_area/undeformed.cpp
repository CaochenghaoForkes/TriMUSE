#include "properties/derived/interface_area/undeformed.hpp"

namespace trimuse
{

UndeformedInterfaceArea::UndeformedInterfaceArea(const ModelParameterMap&)
{
}

StateValue UndeformedInterfaceArea::evaluate(const PropertiesStatePoint& point) const
{
    return point.scalar("precomputed.initial_interface_area");
}

}
