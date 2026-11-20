#pragma once

#include "fem/fem_system.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

inline void initialize_initial_interface_area(PropertiesSystem& properties, const FemSystem& fem)
{
    auto& values = properties.field_data().interface_values.at("precomputed.initial_interface_area");
    const auto& mesh = fem.mesh_data().mfem_mesh();
    mfem::IsoparametricTransformation transformation;
    for (const auto& interface : fem.mesh_data().interfaces())
    {
        mesh.GetFaceTransformation(interface.face, &transformation);
        const auto* rule = fem.boundary_quadrature_rule(transformation.GetGeometryType());
        double area = 0.0;
        for (int q = 0; q < rule->GetNPoints(); ++q)
        {
            const auto& point = rule->IntPoint(q);
            transformation.SetIntPoint(&point);
            area += point.weight * transformation.Weight() * fem.geometric_weight(transformation, point); // 面片代表的初始物理面积，包含降维几何权重
        }
        values[interface.face] = area;
    }
}

}
