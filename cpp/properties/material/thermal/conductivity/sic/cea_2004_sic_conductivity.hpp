#pragma once

#include "properties/material/thermal/conductivity/conductivity_model_manager.hpp"

namespace trimuse
{

// I-NERI 2004报告中的CEA CVD beta-SiC导热率模型
// 模型始终应用饱和辐照修正和孔隙率修正
//
// Reference:
// D. Petti, P. Martin, M. Phelip, and R. Ballinger,
// Development of Improved Models and Designs for Coated-Particle Gas Reactor Fuels,
// Technical Report INEEL/EXT-05-02615, Idaho National Laboratory, 2004
// https://doi.org/10.2172/911237
class Cea2004SicConductivity final : public ConductivityModel
{
public:
    explicit Cea2004SicConductivity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;
};

}
