#pragma once

#include "properties/material/thermal/conductivity/conductivity_model_manager.hpp"

namespace trimuse
{

// I-NERI 2004报告收录的英国Buffer导热率模型
// 模型通过孔隙体积分数计算Buffer导热率
//
// Reference:
// D. Petti, P. Martin, M. Phelip, and R. Ballinger,
// Development of Improved Models and Designs for Coated-Particle Gas Reactor Fuels,
// Technical Report INEEL/EXT-05-02615, Idaho National Laboratory, 2004
// https://doi.org/10.2172/911237
class Uk2004BufferConductivity final : public ConductivityModel
{
public:
    explicit Uk2004BufferConductivity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;
};

}
