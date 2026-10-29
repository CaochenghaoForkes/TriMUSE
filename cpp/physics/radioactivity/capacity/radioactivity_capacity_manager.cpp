#include "physics/radioactivity/capacity/radioactivity_capacity_manager.hpp"
#include "physics/radioactivity/capacity/unit_capacity.hpp"

namespace trimuse
{

RadioactivityCapacityManager::RadioactivityCapacityManager(const FemSystem& fem, const std::vector<RadioactivityDomainConfig>& configs)
    : RadioactivityDomainManager(fem)
{
    registry_.register_model<UnitCapacity>("unit", {});
    assign(configs.empty() ? defaults(fem, "unit") : configs);
}

}
