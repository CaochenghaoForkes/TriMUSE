#pragma once

#include "physics/mechanics/source/mechanics_source_model.hpp"

namespace trimuse
{

class GravitySource final : public MechanicsSourceModel // 使用积分点质量密度计算 b = rho*g
{
public:
    GravitySource(const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesFieldData& properties, std::size_t point) const override;

private:
    VectorValue acceleration_; // 重力加速度，单位 m/s^2；2D 顺序为 [r,z]，3D 顺序为 [x,y,z]
};

}
