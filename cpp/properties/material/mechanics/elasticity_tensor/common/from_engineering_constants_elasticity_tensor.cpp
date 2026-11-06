#include "properties/material/mechanics/elasticity_tensor/common/from_engineering_constants_elasticity_tensor.hpp"
#include "common/matrix_multiply.hpp"

namespace trimuse
{

ElasticityTensorValue FromEngineeringConstantsElasticityTensor::evaluate(const PropertiesStatePoint& state_point) const
{
    const auto& young = state_point.vector("material.mechanics.young_modulus");
    const auto& poisson = state_point.vector("material.mechanics.poisson_ratio");

    const double er = young[0];
    const double et = young[1];
    const double nu_rt = poisson[0];
    const double nu_tt = poisson[1];
    const double g_rt = state_point.scalar("material.mechanics.shear_modulus");
    const double g_tt = et / (2.0 * (1.0 + nu_tt));

    const double eta = et / er;
    const double delta = 1.0 - nu_tt - 2.0 * eta * nu_rt * nu_rt;

    const double c11 = er * (1.0 - nu_tt) / delta;
    const double c12 = et * nu_rt / delta;
    const double c22 = et * (1.0 - eta * nu_rt * nu_rt) / ((1.0 + nu_tt) * delta);
    const double c23 = et * (nu_tt + eta * nu_rt * nu_rt) / ((1.0 + nu_tt) * delta);

    // 材料方向 1 为厚度方向，2、3 为切向；张量剪应变对应的系数为 2G
    const double local[6][6] = {
        {c11, c12, c12, 0.0, 0.0, 0.0},
        {c12, c22, c23, 0.0, 0.0, 0.0},
        {c12, c23, c22, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 2.0 * g_tt, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 2.0 * g_rt, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.0, 2.0 * g_rt}
    };

    const auto& transform = state_point.elasticity_tensor("precomputed.tensor_transform");
    const auto& inverse = state_point.elasticity_tensor("precomputed.tensor_transform_inverse");

    // C_global = T C_local T^{-1}
    double intermediate[6][6];
    matrix_multiply(transform.data, local, intermediate);
    ElasticityTensorValue global;
    matrix_multiply(intermediate, inverse.data, global.data);
    return global;
}

}
