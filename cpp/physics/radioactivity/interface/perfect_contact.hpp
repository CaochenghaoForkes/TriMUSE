#pragma once
#include "physics/radioactivity/interface/radioactivity_interface_model.hpp"

namespace trimuse
{

// 连续场已经共享界面自由度；分域场登记两侧浓度相等的约束。
class RadioactivityPerfectContact final : public RadioactivityInterfaceModel
{
public:
    explicit RadioactivityPerfectContact(const ModelParameterMap&) {}

    void assemble(const InterfaceContext& context, DofConstraints& constraints) const override
    {
        constraints.connect(context.interface());
    }
};

}
