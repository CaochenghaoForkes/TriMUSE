#pragma once

#include "properties/material/thermal/conductivity/conductivity_model_manager.hpp"

namespace trimuse
{

// BISON 2021采用的UCO kernel导热率代理模型
// 该相关式来源于UO2实验数据, 并在缺少UCO数据时假定适用于UCO
//
// Reference:
// W. Jiang et al., TRISO particle fuel performance and failure analysis with BISON,
// Journal of Nuclear Materials 548 (2021) 152795
// https://doi.org/10.1016/j.jnucmat.2021.152795
class Bison2021UcoKernelConductivity final : public ConductivityModel
{
public:
    explicit Bison2021UcoKernelConductivity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;
};

}
