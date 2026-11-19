#pragma once

#include "physics/mechanics/boundary/mechanics_boundary_model.hpp"

namespace trimuse
{

class MechanicsFromSurfacePressure final : public MechanicsBoundaryModel // 按边界分区读取压力标量，单位 Pa，组装时再按积分点外法向计算面力
{
public:
    explicit MechanicsFromSurfacePressure(const ModelParameterMap&);
    MechanicsBoundaryType type() const override { return MechanicsBoundaryType::Pressure; }
    MechanicsBoundaryValue evaluate(const PropertiesFieldData& properties, int zone) const override;
};

}
