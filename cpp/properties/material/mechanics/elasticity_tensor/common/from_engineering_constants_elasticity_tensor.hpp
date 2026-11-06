#pragma once

#include "properties/material/mechanics/elasticity_tensor/elasticity_tensor_model_manager.hpp"

namespace trimuse
{

// 按横观各向同性工程常数构造局部矩阵，再转换到当前积分点的计算坐标系
class FromEngineeringConstantsElasticityTensor final : public ElasticityTensorModel
{
public:
    explicit FromEngineeringConstantsElasticityTensor(const ModelParameterMap&) {}

    ElasticityTensorValue evaluate(const PropertiesStatePoint& state_point) const override;
};

}
