#pragma once
#include "physics/radioactivity/common/radioactivity_domain_manager.hpp"

namespace trimuse
{

class RadioactivitySourceManager : public RadioactivityDomainManager // 管理显式配置的体积核素源区域
{
public:
    explicit RadioactivitySourceManager(const FemSystem& fem, const std::vector<RadioactivityDomainConfig>& configs);
};

}
