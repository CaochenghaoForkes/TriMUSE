#pragma once
#include "physics/thermal/common/thermal_domain_manager.hpp"
namespace trimuse
{

class ThermalCapacityManager : public ThermalDomainManager
{
public:
    explicit ThermalCapacityManager(const FemSystem& fem, const std::vector<ThermalDomainConfig>& configs = {});
};

}
