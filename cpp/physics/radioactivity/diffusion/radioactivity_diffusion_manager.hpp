#pragma once
#include "physics/radioactivity/common/radioactivity_domain_manager.hpp"

namespace trimuse
{

class RadioactivityDiffusionManager : public RadioactivityDomainManager // 注册扩散模型，复用区域系数更新与缓存标记逻辑
{
public:
    explicit RadioactivityDiffusionManager(const FemSystem& fem, const std::vector<RadioactivityDomainConfig>& configs = {});
};

}
