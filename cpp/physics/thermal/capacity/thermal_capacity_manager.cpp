#include "physics/thermal/capacity/thermal_capacity_manager.hpp"
#include "physics/thermal/capacity/rho_cp_capacity.hpp"
namespace trimuse
{

ThermalCapacityManager::ThermalCapacityManager(const FemSystem& fem, const std::vector<ThermalDomainConfig>& configs)
    : ThermalDomainManager(fem)
{
    registry_.register_model<RhoCpCapacity>("rho_cp", {"material.general.mass_density", "material.thermal.specific_heat"});
    assign(configs.empty() ? defaults(fem, "rho_cp") : configs);
}

}
