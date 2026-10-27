#pragma once
#include "physics/thermal/common/thermal_domain_manager.hpp"
namespace trimuse
{

class ThermalSourceManager : public ThermalDomainManager
{
public:
    explicit ThermalSourceManager(const FemSystem& fem, const std::vector<ThermalDomainConfig>& configs);
};

}
