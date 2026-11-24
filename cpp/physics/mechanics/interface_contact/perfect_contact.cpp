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
    constraints.connect(context.interface()); // 按分量合并分域场两侧的界面自由度
}

}
