#pragma once
#include "physics/thermal/boundary/thermal_boundary_model.hpp"
namespace trimuse
{

class ThermalDirichletBoundary final : public ThermalBoundaryModel // 一类边界条件：指定表面温度
{
public:
    explicit ThermalDirichletBoundary(const ModelParameterMap&) {}

    bool essential() const override { return true; } // 标识为本质边界条件，由组装层约束边界自由度

    ThermalBoundaryValues evaluate(const PropertiesFieldData& properties, int zone) const override // 一类边界不修改矩阵与右端项而是直接施加约束
    {
        return {0.0, 0.0, scalar(properties, "external.surface_temperature", zone)}; // 不提供边界矩阵和载荷积分项，仅返回指定表面温度
    }
};

}
