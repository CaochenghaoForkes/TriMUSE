#pragma once

#include "common/model_parameters.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

class MechanicsBodyForceModel // 模型返回求积点上的体积力向量，单位 N/m^3，分量沿求解坐标方向
{
public:
    virtual ~MechanicsBodyForceModel() = default;

    virtual VectorValue evaluate(const PropertiesFieldData& properties, std::size_t point) const = 0;
};

}
