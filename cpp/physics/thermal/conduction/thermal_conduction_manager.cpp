#include "physics/thermal/conduction/thermal_conduction_manager.hpp"
#include "physics/thermal/conduction/fourier_conduction.hpp"
namespace trimuse
{
    
ThermalConductionManager::ThermalConductionManager(const FemSystem& fem, const std::vector<ThermalDomainConfig>& configs) // 导热管理器的构造函数 接收有限元系统 fem 和区域模型配置列表 configs
    : ThermalDomainManager(fem) // 先调用父类构造函数，初始化父类负责的成员
{
    registry_.register_model<FourierConduction>("fourier", {"material.thermal.conductivity"}); // 向继承来的注册表登记 模型名 具体类 依赖属性
    assign(configs.empty() ? defaults(fem, "fourier") : configs); // 未提供配置: 所有区域采用默认 Fourier 模型 提供配置 使用指定的配置列表
}

}
