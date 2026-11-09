#pragma once

#include "physics/mechanics/stiffness/mechanics_stiffness_model.hpp"

namespace trimuse
{

class LinearElasticStiffness final : public MechanicsStiffnessModel
{
public:
    explicit LinearElasticStiffness(const ModelParameterMap&) {}

    ElasticityTensorValue evaluate(const PropertiesFieldData& properties, std::size_t point) const override; // 取得全局积分点数组中 point 处的弹性张量
};

}
