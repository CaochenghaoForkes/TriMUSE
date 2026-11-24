#pragma once

#include "fem/fem_system.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

inline void initialize_initial_face_quadrature_weight(PropertiesSystem& properties, const FemSystem& fem)
{
    auto& values = properties.field_data().face_integration_point_values.at("precomputed.initial_face_quadrature_weight");
    const auto& mesh = fem.mesh_data().mfem_mesh();
    values.resize(fem.face_quadrature_point_count());
    mfem::IsoparametricTransformation transformation;
    for (int face = 0; face < mesh.GetNumFaces(); ++face)
    {
        mesh.GetFaceTransformation(face, &transformation);
        const auto* rule = fem.boundary_quadrature_rule(transformation.GetGeometryType());
        const std::size_t offset = fem.face_quadrature_offset(face);
        for (int q = 0; q < rule->GetNPoints(); ++q)
        {
            const auto& point = rule->IntPoint(q);
            transformation.SetIntPoint(&point);
            values[offset + q] = point.weight * transformation.Weight() * fem.geometric_weight(transformation, point); // 初始面求积点物理面积权重 wq |JΓ,q| gq
        }
    }
}

}
