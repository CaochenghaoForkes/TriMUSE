#pragma once

#include "physics/mechanics/boundary/mechanics_boundary_model.hpp"

namespace trimuse
{

class MechanicsFromSurfaceTraction final : public MechanicsBoundaryModel // 读取指定面力，单位 Pa，分量沿求解坐标方向
{
public:
    explicit MechanicsFromSurfaceTraction(const ModelParameterMap&) {}

    MechanicsBoundaryType type() const override { return MechanicsBoundaryType::Traction; }

    MechanicsBoundaryValue evaluate(const PropertiesFieldData& properties, int zone) const override; // 取得指定区域的面力
};

}
