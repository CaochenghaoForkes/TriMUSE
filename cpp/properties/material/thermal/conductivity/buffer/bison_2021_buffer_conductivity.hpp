#pragma once

#include "properties/material/thermal/conductivity/conductivity_model_manager.hpp"

namespace trimuse
{

// BISON 2021采用的Buffer密度相关导热率模型
// 该模型取自PARFUME, 通过当前密度描述Buffer致密化对导热率的影响
//
// References:
// W. Jiang et al., TRISO particle fuel performance and failure analysis with BISON,
// Journal of Nuclear Materials 548 (2021) 152795
// https://doi.org/10.1016/j.jnucmat.2021.152795
//
// G. K. Miller et al., PARFUME Theory and Model Basis Report,
// Technical Report INL/EXT-08-14497, Idaho National Laboratory, 2009
// https://doi.org/10.2172/968587
class Bison2021BufferConductivity final : public ConductivityModel
{
public:
    explicit Bison2021BufferConductivity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;

private:
    double initial_mass_density_;
    static constexpr double theoretical_mass_density_ = 2250.0; // constexpr 编译期常量 初始化值必须能够在编译时确定 创建以后不能修改 编译器可以直接把它用于常量计算
    static constexpr double initial_conductivity_ = 0.5;
    static constexpr double theoretical_conductivity_ = 4.0;

};

}
