#pragma once

#include "physics/radioactivity/common/radioactivity_domain_manager.hpp"

namespace trimuse
{

class RadioactivityDecayManager : public RadioactivityDomainManager // 为已配置区域提供衰变系数及变化标记
{
public:
    RadioactivityDecayManager(const FemSystem& fem, const std::vector<RadioactivityDomainConfig>& configs);
};

}
