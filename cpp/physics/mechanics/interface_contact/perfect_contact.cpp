#include "physics/mechanics/interface_contact/perfect_contact.hpp"
#include "physics/common/interface/interface_context.hpp"
#include "physics/common/constraint/dof_constraints.hpp"

namespace trimuse
{

MechanicsPerfectContact::MechanicsPerfectContact(const ModelParameterMap&) // 完美粘结不需要额外参数
{
}

void MechanicsPerfectContact::assemble(const InterfaceContext& context, DofConstraints& constraints) const
{
    constraints.connect(context.interface()); // 分域场按分量合并界面自由度，连续场已共享自由度
}

}
