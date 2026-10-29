#pragma once
#include "physics/radioactivity/common/radioactivity_domain_manager.hpp"

namespace trimuse
{

class RadioactivityDiffusionManager : public RadioactivityDomainManager
{
public:
    explicit RadioactivityDiffusionManager(const FemSystem& fem, const std::vector<RadioactivityDomainConfig>& configs = {});
};

}
