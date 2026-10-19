#pragma once

#include "properties/material/thermal/conductivity/conductivity_model_manager.hpp"

namespace trimuse
{

// BISON和PARFUME采用的Miller SiC导热率模型
// 该模型仅依赖SiC当前温度
//
// References:
// W. Jiang et al., TRISO particle fuel performance and failure analysis with BISON,
// Journal of Nuclear Materials 548 (2021) 152795
// https://doi.org/10.1016/j.jnucmat.2021.152795
//
// G. K. Miller et al., PARFUME Theory and Model Basis Report,
// Technical Report INL/EXT-08-14497, Idaho National Laboratory
// https://doi.org/10.2172/968587
class MillerSicConductivity final : public ConductivityModel
{
public:
    explicit MillerSicConductivity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;
};

}
