#include "physics/radioactivity/interface_nuclide_transfer/perfect_contact.hpp"

namespace trimuse
{

RadioactivityPerfectContact::RadioactivityPerfectContact(const ModelParameterMap&)
{
}

void RadioactivityPerfectContact::assemble(const InterfaceContext& context, DofConstraints& constraints) const // 将当前面片两侧同位置的浓度自由度并入同组
{
    constraints.connect(context.interface());
}

}
