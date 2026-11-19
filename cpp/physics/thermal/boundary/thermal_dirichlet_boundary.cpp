#include "physics/thermal/boundary/thermal_dirichlet_boundary.hpp"

namespace trimuse
{

ThermalDirichletBoundary::ThermalDirichletBoundary(const ModelParameterMap&)
{
}

ThermalBoundaryValues ThermalDirichletBoundary::evaluate(const PropertiesFieldData& properties, int zone) const // 一类边界不修改矩阵与右端项而是直接施加约束
{
    return {0.0, 0.0, scalar(properties, "external.surface_temperature", zone)}; // 不提供边界矩阵和载荷积分项，仅返回指定表面温度
}

}
