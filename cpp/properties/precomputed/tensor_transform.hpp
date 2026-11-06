#pragma once

#include "fem/fem_system.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

// 对称二阶张量的六分量顺序为 [11,22,33,23,13,12]，剪应变使用张量分量
inline void initialize_tensor_transform(PropertiesFieldData& properties, const FemSystem& fem)
{
    auto& transforms = properties.integration_point_values.at("precomputed.tensor_transform");
    auto& inverses = properties.integration_point_values.at("precomputed.tensor_transform_inverse");
    transforms.resize(fem.domain_quadrature_point_count());
    inverses.resize(fem.domain_quadrature_point_count());
    for (std::size_t point = 0; point < transforms.size(); ++point)
    {
        const auto& q = fem.material_basis(point);
        const double q11 = q.data[0][0], q12 = q.data[0][1], q13 = q.data[0][2];
        const double q21 = q.data[1][0], q22 = q.data[1][1], q23 = q.data[1][2];
        const double q31 = q.data[2][0], q32 = q.data[2][1], q33 = q.data[2][2];

        // A_global = Q A_local Q^T -> a_global = T a_local
        const ElasticityTensorValue transform{{
            {q11*q11, q12*q12, q13*q13, 2*q12*q13, 2*q11*q13, 2*q11*q12},
            {q21*q21, q22*q22, q23*q23, 2*q22*q23, 2*q21*q23, 2*q21*q22},
            {q31*q31, q32*q32, q33*q33, 2*q32*q33, 2*q31*q33, 2*q31*q32},
            {q21*q31, q22*q32, q23*q33, q22*q33+q23*q32, q21*q33+q23*q31, q21*q32+q22*q31},
            {q11*q31, q12*q32, q13*q33, q12*q33+q13*q32, q11*q33+q13*q31, q11*q32+q12*q31},
            {q11*q21, q12*q22, q13*q23, q12*q23+q13*q22, q11*q23+q13*q21, q11*q22+q12*q21}
        }};

        // A_local = Q^T A_global Q -> a_local = T^{-1} a_global
        // 将上式中的 Q 换成 Q^T，逐项展开；T^{-1} 一般不等于 T^T
        const ElasticityTensorValue inverse{{
            {q11*q11, q21*q21, q31*q31, 2*q21*q31, 2*q11*q31, 2*q11*q21},
            {q12*q12, q22*q22, q32*q32, 2*q22*q32, 2*q12*q32, 2*q12*q22},
            {q13*q13, q23*q23, q33*q33, 2*q23*q33, 2*q13*q33, 2*q13*q23},
            {q12*q13, q22*q23, q32*q33, q22*q33+q32*q23, q12*q33+q32*q13, q12*q23+q22*q13},
            {q11*q13, q21*q23, q31*q33, q21*q33+q31*q23, q11*q33+q31*q13, q11*q23+q21*q13},
            {q11*q12, q21*q22, q31*q32, q21*q32+q31*q22, q11*q32+q31*q12, q11*q22+q21*q12}
        }};
        transforms[point] = transform;
        inverses[point] = inverse;
    }
}

}
