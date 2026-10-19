#pragma once

#include "properties/material/thermal/specific_heat/specific_heat_model_manager.hpp"

namespace trimuse
{

// UO2 摩尔热容相关式, 以 U-235 富集度计算摩尔质量后换算为质量比热容
//
// References:
// J. Fink, Thermophysical properties of uranium dioxide,
// Journal of Nuclear Materials 279 (2000) 1-18.
// W. Jiang et al., TRISO particle fuel performance and failure analysis with BISON,
// Journal of Nuclear Materials 548 (2021) 152795, Eq. (2)-(5).
// https://doi.org/10.1016/j.jnucmat.2021.152795
class Fink2000Uo2SpecificHeat final : public SpecificHeatModel
{
public:
    explicit Fink2000Uo2SpecificHeat(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;

private:
    double molar_mass_; // kg/mol
};

}
