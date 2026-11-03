#pragma once
#include "physics/radioactivity/common/radioactivity_domain_manager.hpp"

namespace trimuse
{

class RadioactivityCapacityManager : public RadioactivityDomainManager // 为单位容量矩阵提供积分点系数
{
public:
    explicit RadioactivityCapacityManager(const FemSystem& fem, const std::vector<RadioactivityDomainConfig>& configs = {});
};

}
