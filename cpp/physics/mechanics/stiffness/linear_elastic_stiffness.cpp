#include "physics/mechanics/stiffness/linear_elastic_stiffness.hpp"

namespace trimuse
{

ElasticityTensorValue LinearElasticStiffness::evaluate(const PropertiesFieldData& properties, std::size_t point) const // 取得全局积分点数组中 point 处的弹性张量
{
    return std::get<ElasticityTensorValue>(properties.integration_point_values.at("material.mechanics.elasticity_tensor")[point].value());
}

}
