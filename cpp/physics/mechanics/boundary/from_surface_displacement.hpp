#pragma once

#include "physics/mechanics/boundary/mechanics_boundary_model.hpp"

namespace trimuse
{

class MechanicsFromSurfaceDisplacement final : public MechanicsBoundaryModel // 读取指定位移，单位 m，由组装层约束全部位移分量
{
public:
    explicit MechanicsFromSurfaceDisplacement(const ModelParameterMap&) {}

    MechanicsBoundaryType type() const override { return MechanicsBoundaryType::Displacement; }

    MechanicsBoundaryValue evaluate(const PropertiesFieldData& properties, int zone) const override;
};

}
