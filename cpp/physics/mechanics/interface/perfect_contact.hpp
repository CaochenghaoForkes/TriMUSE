#pragma once

#include "physics/mechanics/interface/mechanics_interface_model.hpp"

namespace trimuse
{

class MechanicsPerfectContact final : public MechanicsInterfaceModel // 完美粘结：两侧对应位置的各个位移分量相等
{
public:
    explicit MechanicsPerfectContact(const ModelParameterMap& parameters);
    void assemble(const InterfaceContext& context, DofConstraints& constraints) const override;
};

}
