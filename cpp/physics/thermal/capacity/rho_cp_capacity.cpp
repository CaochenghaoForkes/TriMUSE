#include "physics/thermal/capacity/rho_cp_capacity.hpp"

namespace trimuse
{

RhoCpCapacity::RhoCpCapacity(const ModelParameterMap&)
{
}

double RhoCpCapacity::evaluate(const PropertiesFieldData& properties, std::size_t point) const
{
    return scalar(properties, "material.general.mass_density", point) * scalar(properties, "material.thermal.specific_heat", point);
}

}
