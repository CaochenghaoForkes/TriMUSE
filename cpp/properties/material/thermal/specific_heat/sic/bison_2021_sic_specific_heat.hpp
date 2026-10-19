#pragma once

#include "properties/material/thermal/specific_heat/specific_heat_model_manager.hpp"

namespace trimuse
{

// BISON 2021 使用的 SiC 温度相关比热容, 输入温度为 K, 返回 J/(kg K)
//
// Reference:
// W. Jiang et al., TRISO particle fuel performance and failure analysis with BISON,
// Journal of Nuclear Materials 548 (2021) 152795, Eq. (31).
// https://doi.org/10.1016/j.jnucmat.2021.152795
class Bison2021SicSpecificHeat final : public SpecificHeatModel
{
public:
    explicit Bison2021SicSpecificHeat(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;
};

}
