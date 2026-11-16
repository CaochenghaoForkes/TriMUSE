#include "physics/mechanics/source/gravity.hpp"

namespace trimuse
{

GravitySource::GravitySource(const ModelParameterMap& parameters)
{
    acceleration_ = std::get<VectorValue>(parameters.at("acceleration"));
}

VectorValue GravitySource::evaluate(const PropertiesFieldData& properties, std::size_t point) const
{
    const double density = std::get<double>(properties.integration_point_values.at("material.general.mass_density")[point].value());
    VectorValue force = acceleration_;
    for (double& component : force) 
    { 
        component *= density; 
    }
    return force;
}

}
