#pragma once
#include "physics/radioactivity/interface_nuclide_transfer/radioactivity_interface_nuclide_transfer_model.hpp"

namespace trimuse
{

// 登记分域场两侧浓度相等的约束。
class RadioactivityPerfectContact final : public RadioactivityInterfaceNuclideTransferModel
{
public:
    explicit RadioactivityPerfectContact(const ModelParameterMap&);
    void assemble(const InterfaceContext& context, DofConstraints& constraints) const override; // 将当前面片两侧同位置的浓度自由度并入同组
};

}
