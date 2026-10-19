#include "properties/material/thermal/conductivity/buffer/uk_2004_buffer_conductivity.hpp"

namespace trimuse
{

Uk2004BufferConductivity::Uk2004BufferConductivity(const ModelParameterMap&)
{
}

double Uk2004BufferConductivity::evaluate(const PropertiesStatePoint& state_point) const
{
    const double porosity = state_point.scalar("general.porosity");
    const double conductivity = 10.98222 * (1.0 - porosity) / (1.0 + 2.0 * porosity) + 0.00444;
    return checked_conductivity(conductivity);
}

}
