#pragma once

#include "physics/mechanics/common/mechanics_interface_config.hpp"

namespace trimuse
{

class InterfaceContext;
class DofConstraints;

class MechanicsInterfaceContactModel // 每个模型处理一块界面面片，登记两侧位移的约束关系
{
public:
    virtual ~MechanicsInterfaceContactModel() = default;
    virtual void assemble(const InterfaceContext& context, DofConstraints& constraints) const = 0;
};

}
