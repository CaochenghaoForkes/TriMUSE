#pragma once
#include "physics/radioactivity/common/radioactivity_domain_manager.hpp"

namespace trimuse
{

class RadioactivitySourceManager : public RadioactivityDomainManager
{
public:
    explicit RadioactivitySourceManager(const FemSystem& fem, const std::vector<RadioactivityDomainConfig>& configs);
};

}
