#include "physics/thermal/interface/insulated.hpp"

namespace trimuse
{

void ThermalInsulatedInterface::assemble(const ThermalInterfaceContext&, ThermalInterfaceAssembly&) const
{
    // 绝热界面不连接两侧自由度, 也没有热流积分贡献
}

}
