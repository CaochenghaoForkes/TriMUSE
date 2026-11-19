#include "physics/mechanics/source/gravity.hpp"

namespace trimuse
{

GravitySource::GravitySource(const ModelParameterMap&)
{
}

VectorValue GravitySource::evaluate(const PropertiesFieldData& properties, std::size_t point) const
{
    const double density = std::get<double>(properties.integration_point_values.at("material.general.mass_density")[point].value());
    VectorValue force = std::get<VectorValue>(properties.global_values.at("precomputed.gravity_acceleration").value());
    for (double& component : force)
    {
        component *= density;
    }
    return force;
}

}
