#include "physics/mechanics/thermal_expansion/linear_thermal_expansion.hpp"
#include "common/matrix_multiply.hpp"

namespace trimuse
{

VectorValue LinearThermalExpansion::evaluate(const PropertiesFieldData& properties, std::size_t point, double temperature) const // 取得求积点 point 上向量 \sigma_{th,global} = C:\varepsilon_{th,global}
{
    const auto& coefficients = std::get<VectorValue>(properties.integration_point_values.at("material.mechanics.thermal_expansion_coef")[point].value());
    const double reference_temperature = std::get<double>(properties.global_values.at("precomputed.thermal_expansion_reference_temperature").value());
    const double temperature_difference = temperature - reference_temperature;
    const auto& transform = std::get<ElasticityTensorValue>(properties.integration_point_values.at("precomputed.tensor_transform")[point].value());
    const auto& elasticity = std::get<ElasticityTensorValue>(properties.integration_point_values.at("material.mechanics.elasticity_tensor")[point].value());

    const double local_strain[6][1] = {
        {coefficients[0] * temperature_difference},
        {coefficients[1] * temperature_difference},
        {coefficients[1] * temperature_difference},
        {0.0},
        {0.0},
        {0.0}}; // \varepsilon_{th,local} = diag(alpha_r, alpha_t, alpha_t) * (T - T_ref)
    double global_strain[6][1], thermal_load[6][1];
    matrix_multiply(transform.data, local_strain, global_strain); // \varepsilon_{th,global} = Transform * \varepsilon_{th,local}
    matrix_multiply(elasticity.data, global_strain, thermal_load); // \sigma_{th,global} = C:\varepsilon_{th,global} = C_{global} * \varepsilon_{th,global}
    return {thermal_load[0][0], thermal_load[1][0], thermal_load[2][0], thermal_load[3][0], thermal_load[4][0], thermal_load[5][0]};
}

LinearThermalExpansion::LinearThermalExpansion(const ModelParameterMap&)
{
}

}
