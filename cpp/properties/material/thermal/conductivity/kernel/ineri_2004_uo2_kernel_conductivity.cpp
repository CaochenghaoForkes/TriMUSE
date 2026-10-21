#include "properties/material/thermal/conductivity/kernel/ineri_2004_uo2_kernel_conductivity.hpp"

namespace trimuse
{

Ineri2004Uo2KernelConductivity::Ineri2004Uo2KernelConductivity(const ModelParameterMap&)
{
}

double Ineri2004Uo2KernelConductivity::evaluate(const PropertiesStatePoint& state_point) const
{
    const double temperature = state_point.scalar("temperature");
    const double burnup_percent = 100.0 * state_point.scalar("burnup_fima_fraction");
    const double porosity = state_point.scalar("general.porosity");
    const double base_conductivity = 1.0 / (0.0375 + 2.165e-4 * temperature) + 4.715e9 / std::pow(temperature, 2) * std::exp(-16361.0 / temperature);
    double k_1d = 1.0;
    double k_1p = 1.0;
    double k_4r = 1.0;
    if (burnup_percent > 0.0)
    {
        const double a = 1.09 / std::pow(burnup_percent, 3.265) + 0.0643 * std::sqrt(temperature) / std::sqrt(burnup_percent);
        k_1d = a * std::atan(1.0 / a);
        k_1p = 1.0 + 0.019 * burnup_percent / (3.0 - 0.019 * burnup_percent) / (1.0 + std::exp(-(temperature - 1200.0) / 100.0));
        k_4r = 1.0 - 0.2 / (1.0 + std::exp((temperature - 900.0) / 80.0)) * (1.0 - std::exp(-burnup_percent));
    }
    const double k_2p = (1.0 - porosity) / (1.0 + 2.0 * porosity);
    const double conductivity = base_conductivity * k_1d * k_1p * k_2p * k_4r;
    return checked_conductivity(conductivity);
}

}
