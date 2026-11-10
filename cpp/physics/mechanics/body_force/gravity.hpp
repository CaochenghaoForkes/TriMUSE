#pragma once

#include "physics/mechanics/body_force/mechanics_body_force_model.hpp"

namespace trimuse
{

class GravityBodyForce final : public MechanicsBodyForceModel // 使用积分点质量密度计算 b = rho*g
{
public:
    GravityBodyForce(const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesFieldData& properties, std::size_t point) const override;

private:
    VectorValue acceleration_; // 重力加速度，单位 m/s^2；2D 顺序为 [r,z]，3D 顺序为 [x,y,z]
};

}
