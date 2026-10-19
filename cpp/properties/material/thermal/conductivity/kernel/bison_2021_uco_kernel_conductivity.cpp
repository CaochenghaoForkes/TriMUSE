#include "properties/material/thermal/conductivity/kernel/bison_2021_uco_kernel_conductivity.hpp"

namespace trimuse
{

Bison2021UcoKernelConductivity::Bison2021UcoKernelConductivity(const ModelParameterMap&)
{
}

double Bison2021UcoKernelConductivity::evaluate(const PropertiesStatePoint& state_point) const
{
    const double temperature_celsius = state_point.scalar("temperature") - 273.15;
    const double exponential_term = 0.0132 * std::exp(0.00188 * temperature_celsius);
    const double conductivity = exponential_term + (temperature_celsius < 1650.0 ? 4040.0 / (464.0 + temperature_celsius) : 1.9);
    return checked_conductivity(conductivity);
}

}
