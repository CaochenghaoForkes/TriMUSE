#include "properties/material/thermal/conductivity/buffer/bison_2021_buffer_conductivity.hpp"

namespace trimuse
{

Bison2021BufferConductivity::Bison2021BufferConductivity(const ModelParameterMap& parameters)
    : initial_mass_density_(std::get<double>(parameters.at("initial_mass_density")))
{
}

double Bison2021BufferConductivity::evaluate(const PropertiesStatePoint& state_point) const
{
    const double current_mass_density = state_point.scalar("general.mass_density");
    if (current_mass_density < initial_mass_density_ || current_mass_density > theoretical_mass_density_)
    {
        throw std::domain_error("Buffer 当前密度超出致密化模型范围");
    }
    const double numerator = initial_conductivity_ * theoretical_conductivity_ * theoretical_mass_density_
        * (theoretical_mass_density_ - initial_mass_density_);
    const double denominator = theoretical_conductivity_ * theoretical_mass_density_ * (theoretical_mass_density_ - current_mass_density)
        + initial_conductivity_ * current_mass_density * (current_mass_density - initial_mass_density_);
    return checked_conductivity(numerator / denominator);
}

}
