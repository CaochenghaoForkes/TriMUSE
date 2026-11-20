#pragma once

#include "fem/fem_system.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

inline void initialize_initial_domain_quadrature_weight(PropertiesSystem& properties, const FemSystem& fem)
{
    auto& values = properties.field_data().integration_point_values.at("precomputed.initial_domain_quadrature_weight");
    values.resize(fem.domain_quadrature_point_count());
    const auto& mesh = fem.mesh_data().mfem_mesh();
    mfem::IsoparametricTransformation transformation;
    for (int element = 0; element < mesh.GetNE(); ++element)
    {
        mesh.GetElementTransformation(element, &transformation);
        const auto* rule = fem.domain_quadrature_rule(mesh.GetElementBaseGeometry(element));
        const std::size_t offset = fem.domain_quadrature_offset(element);
        for (int q = 0; q < rule->GetNPoints(); ++q)
        {
            const auto& point = rule->IntPoint(q);
            transformation.SetIntPoint(&point);
            values[offset + q] = point.weight * transformation.Weight() * fem.geometric_weight(transformation, point); // 初始物理体积权重 wq |Jq| gq
        }
    }
}

}
