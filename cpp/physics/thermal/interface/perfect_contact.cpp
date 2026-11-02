#include "physics/thermal/interface/perfect_contact.hpp"
#include "physics/thermal/interface/thermal_interface_assembly.hpp"

namespace trimuse
{

void ThermalPerfectContact::assemble(const InterfaceContext& context, ThermalInterfaceAssembly& assembly) const // 分域场：配对当前界面片两侧自由度，并在 representative_ 中合并分组 连续场：已共享界面自由度，无需额外连接
{
    assembly.connect(context);
}

}
