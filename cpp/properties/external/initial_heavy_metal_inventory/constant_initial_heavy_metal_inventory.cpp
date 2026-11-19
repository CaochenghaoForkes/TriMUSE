#include "properties/external/initial_heavy_metal_inventory/constant_initial_heavy_metal_inventory.hpp"

namespace trimuse
{

ConstantInitialHeavyMetalInventory::ConstantInitialHeavyMetalInventory(const ModelParameterMap& parameters)
    :
    ConstantModel(parameters)
{
}

StateValue ConstantInitialHeavyMetalInventory::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
