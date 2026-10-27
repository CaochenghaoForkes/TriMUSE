#pragma once
#include "physics/thermal/common/thermal_domain_manager.hpp"
namespace trimuse
{

class ThermalConductionManager : public ThermalDomainManager
{
public:
    explicit ThermalConductionManager(const FemSystem& fem, const std::vector<ThermalDomainConfig>& configs = {}); // 导热管理器的构造函数 接收有限元系统 fem 和区域模型配置列表 configs
};

}
