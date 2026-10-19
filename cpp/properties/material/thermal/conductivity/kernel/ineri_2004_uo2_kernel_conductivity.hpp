#pragma once

#include "properties/material/thermal/conductivity/conductivity_model_manager.hpp"

namespace trimuse
{

// I-NERI 2004报告中的UO2 kernel导热率模型
// 模型考虑温度, 燃耗和孔隙率的影响
//
// Reference:
// D. Petti, P. Martin, M. Phelip, and R. Ballinger,
// Development of Improved Models and Designs for Coated-Particle Gas Reactor Fuels,
// Technical Report INEEL/EXT-05-02615, Idaho National Laboratory, 2004
// https://doi.org/10.2172/911237
class Ineri2004Uo2KernelConductivity final : public ConductivityModel
{
public:
    explicit Ineri2004Uo2KernelConductivity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;
};

}
