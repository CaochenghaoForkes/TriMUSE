#pragma once
#include "physics/radioactivity/common/radioactivity_domain_manager.hpp"

namespace trimuse
{

class RadioactivityCapacityManager : public RadioactivityDomainManager
{
public:
    explicit RadioactivityCapacityManager(const FemSystem& fem, const std::vector<RadioactivityDomainConfig>& configs = {});
};

}
