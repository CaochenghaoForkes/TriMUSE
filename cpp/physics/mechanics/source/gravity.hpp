#pragma once

#include "physics/mechanics/source/mechanics_source_model.hpp"

namespace trimuse
{

class GravitySource final : public MechanicsSourceModel // 使用积分点质量密度计算 b = rho*g
{
public:
    GravitySource(const ModelParameterMap&);
    VectorValue evaluate(const PropertiesFieldData& properties, std::size_t point) const override;
};

}
