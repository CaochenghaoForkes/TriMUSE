#include "fem/fem_system.hpp"

#include <algorithm>

namespace trimuse
{

FemSystem::FemSystem(const std::string& mesh_path, int domain_quadrature_order, int boundary_quadrature_order)
    : mesh_data_(mesh_path),
      domain_quadrature_rules_(make_quadrature_rules(mesh_data_, mesh_data_.dimension(), domain_quadrature_order)),
      boundary_quadrature_rules_(make_quadrature_rules(mesh_data_, mesh_data_.dimension() - 1, boundary_quadrature_order))
{
    if (mesh_data_.dimension() < 3)
    {
        const mfem::Mesh& mesh = mesh_data_.mfem_mesh();
        radial_origin_x_ = mesh.GetVertex(0)[0];
        for (int vertex = 1; vertex < mesh.GetNV(); ++vertex)
        {
            radial_origin_x_ = std::min(radial_origin_x_, mesh.GetVertex(vertex)[0]);
        }
    }
}

FemSystem::FieldObjects::FieldObjects(MeshData& mesh_data, int order, int vector_dimension)
    : finite_element_space(mesh_data, order, vector_dimension),
      field_data(finite_element_space)
{
}

FemSystem::QuadratureRules FemSystem::make_quadrature_rules(const MeshData& mesh_data, int dimension, int order)
{
    QuadratureRules rules;
    mfem::Array<mfem::Geometry::Type> geometries;
    mesh_data.mfem_mesh().GetGeometries(dimension, geometries);
    for (const mfem::Geometry::Type geometry : geometries)
    {
        rules.emplace(geometry, &mfem::IntRules.Get(geometry, order));
    }
    return rules;
}

void FemSystem::add_field(const std::string& name, int order, int vector_dimension)
{
    field_map_.emplace(name, std::make_unique<FieldObjects>(mesh_data_, order, vector_dimension));
}

MeshData& FemSystem::mesh_data()
{
    return mesh_data_;
}

const MeshData& FemSystem::mesh_data() const
{
    return mesh_data_;
}

FiniteElementSpaceData& FemSystem::finite_element_space(const std::string& name)
{
    return field_map_.at(name)->finite_element_space;
}

const FiniteElementSpaceData& FemSystem::finite_element_space(const std::string& name) const
{
    return field_map_.at(name)->finite_element_space;
}

FieldData& FemSystem::field_data(const std::string& name)
{
    return field_map_.at(name)->field_data;
}

const FieldData& FemSystem::field_data(const std::string& name) const
{
    return field_map_.at(name)->field_data;
}

const mfem::IntegrationRule* FemSystem::domain_quadrature_rule(mfem::Geometry::Type geometry) const
{
    return domain_quadrature_rules_.at(geometry);
}

const mfem::IntegrationRule* FemSystem::boundary_quadrature_rule(mfem::Geometry::Type geometry) const
{
    return boundary_quadrature_rules_.at(geometry);
}

double FemSystem::geometric_weight(mfem::ElementTransformation& transformation, const mfem::IntegrationPoint& point) const
{
    const int dimension = mesh_data_.dimension();
    if (dimension == 3)
    {
        return 1.0;
    }
    constexpr double pi = 3.14159265358979323846;
    double coordinates[3];
    mfem::Vector position(coordinates, mesh_data_.space_dimension());
    transformation.Transform(point, position);
    const double radius = position[0] - radial_origin_x_;
    return dimension == 1 ? 4.0 * pi * radius * radius : 2.0 * pi * radius;
}

}
