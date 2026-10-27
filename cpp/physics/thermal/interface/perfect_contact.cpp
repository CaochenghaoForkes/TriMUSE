#include "physics/thermal/interface/perfect_contact.hpp"
#include "physics/thermal/interface/thermal_interface_assembly.hpp"

namespace trimuse
{

void ThermalPerfectContact::assemble(const ThermalInterfaceContext& context, ThermalInterfaceAssembly& assembly) const
{
    assembly.connect(context);
}

}
