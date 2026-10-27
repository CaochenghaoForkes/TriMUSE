#include "physics/thermal/source/thermal_source_manager.hpp"
#include "physics/thermal/source/volumetric_power_source.hpp"
namespace trimuse
{

ThermalSourceManager::ThermalSourceManager(const FemSystem& fem, const std::vector<ThermalDomainConfig>& configs)
    : ThermalDomainManager(fem)
{
    registry_.register_model<VolumetricPowerSource>("volumetric_power", {"derived.volumetric_power"});
    assign(configs);
}

}
