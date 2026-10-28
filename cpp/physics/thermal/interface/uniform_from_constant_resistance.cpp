#include "physics/thermal/interface/uniform_from_constant_resistance.hpp"
#include "physics/thermal/interface/thermal_interface_assembly.hpp"

#include <cmath>
#include <stdexcept>

namespace trimuse
{

ThermalUniformFromConstantResistance::ThermalUniformFromConstantResistance(const ModelParameterMap& parameters)
    : resistance_(std::get<double>(parameters.at("constant_resistance")))
{
    if (!std::isfinite(resistance_) || resistance_ <= 0.0)
    {
        throw std::invalid_argument("constant_resistance 必须为有限正数, 单位 m²·K/W");
    }
}

void ThermalUniformFromConstantResistance::assemble(const ThermalInterfaceContext& context, ThermalInterfaceAssembly& assembly) const
{
    mfem::ConstantCoefficient conductance(1.0 / resistance_);
    assembly.add_transfer(context, conductance);
}

}
