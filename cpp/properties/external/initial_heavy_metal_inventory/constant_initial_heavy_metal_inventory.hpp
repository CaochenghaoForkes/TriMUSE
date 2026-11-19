#pragma once

#include "properties/external/initial_heavy_metal_inventory/initial_heavy_metal_inventory_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantInitialHeavyMetalInventory final : public InitialHeavyMetalInventoryModel, private ConstantModel
{
public:
    explicit ConstantInitialHeavyMetalInventory(const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
