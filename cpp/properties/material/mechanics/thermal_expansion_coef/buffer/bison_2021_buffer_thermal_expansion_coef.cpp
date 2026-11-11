#include "properties/material/mechanics/thermal_expansion_coef/buffer/bison_2021_buffer_thermal_expansion_coef.hpp"

namespace trimuse
{

Bison2021BufferThermalExpansionCoef::Bison2021BufferThermalExpansionCoef(const ModelParameterMap&)
{
}

// Jiang et al., Journal of Nuclear Materials 548 (2021) 152795，式（12）
VectorValue Bison2021BufferThermalExpansionCoef::evaluate(const PropertiesStatePoint& state_point) const
{
    const double temperature_celsius = state_point.scalar("temperature") - 273.15;
    const double coefficient = 5e-6 * (1.0 + 0.11 * (temperature_celsius - 400.0) / 700.0);
    return {coefficient, coefficient};
}

}
