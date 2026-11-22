#include "properties/external/initial_heavy_metal_inventory/initial_heavy_metal_inventory_model_manager.hpp"
#include "properties/external/initial_heavy_metal_inventory/constant_initial_heavy_metal_inventory.hpp"

namespace trimuse
{

InitialHeavyMetalInventoryModel::~InitialHeavyMetalInventoryModel() = default;

void InitialHeavyMetalInventoryModelManager::register_models()
{
    registry_.register_model<ConstantInitialHeavyMetalInventory>("common.constant");
}

void InitialHeavyMetalInventoryModelManager::assign(int region_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    region_model_map_[region_attribute] = registry_.registration(model_name).factory(parameters);
}

StateValue InitialHeavyMetalInventoryModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return region_model_map_.at(point.region_attribute)->evaluate(point);
}

const std::vector<RequiredState>& InitialHeavyMetalInventoryModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
