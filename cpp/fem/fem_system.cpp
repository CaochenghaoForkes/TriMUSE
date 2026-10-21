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
    cache_domain_quadrature_positions();
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

void FemSystem::cache_domain_quadrature_positions()
{
    const mfem::Mesh& mesh = mesh_data_.mfem_mesh();
    domain_quadrature_positions_.resize(mesh.GetNE());
    domain_quadrature_offsets_.resize(mesh.GetNE() + 1);
    mfem::Vector position(mesh.SpaceDimension());
    for (int element = 0; element < mesh.GetNE(); ++element)
    {
        const mfem::IntegrationRule* rule = domain_quadrature_rule(mesh.GetElementBaseGeometry(element));
        domain_quadrature_offsets_[element + 1] = domain_quadrature_offsets_[element] + rule->GetNPoints();
        mfem::IsoparametricTransformation transformation;
        mesh.GetElementTransformation(element, &transformation);
        QuadraturePositions& positions = domain_quadrature_positions_[element];
        positions.resize(rule->GetNPoints());
        for (int point_index = 0; point_index < rule->GetNPoints(); ++point_index)
        {
            transformation.Transform(rule->IntPoint(point_index), position);
            positions[point_index] = {position[0], position.Size() > 1 ? position[1] : 0.0, position.Size() > 2 ? position[2] : 0.0};
        }
    }
}

const FemSystem::QuadraturePositions& FemSystem::domain_quadrature_positions(int element) const
{
    return domain_quadrature_positions_[element];
}

std::size_t FemSystem::domain_quadrature_offset(int element) const
{
    return domain_quadrature_offsets_[element];
}

std::size_t FemSystem::domain_quadrature_point_count() const
{
    return domain_quadrature_offsets_.back();
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
