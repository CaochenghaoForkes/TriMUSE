#include "properties/material/thermal/conductivity/sic/miller_sic_conductivity.hpp"

namespace trimuse
{

MillerSicConductivity::MillerSicConductivity(const ModelParameterMap&)
{
}

double MillerSicConductivity::evaluate(const PropertiesStatePoint& state_point) const
{
    return checked_conductivity(17885.0 / state_point.scalar("temperature") + 2.0);
}

}
