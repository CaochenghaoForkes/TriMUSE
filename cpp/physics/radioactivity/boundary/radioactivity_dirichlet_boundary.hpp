#pragma once
#include "physics/radioactivity/boundary/radioactivity_boundary_model.hpp"
namespace trimuse
{

class RadioactivityDirichletBoundary final : public RadioactivityBoundaryModel // 指定外表面浓度，不直接生成矩阵或右端项
{
public:
    explicit RadioactivityDirichletBoundary(const ModelParameterMap&) {}

    bool essential() const override { return true; } // 标记为本质边界，求解系统中将固定对应自由度

    RadioactivityBoundaryValues evaluate(const PropertiesFieldData& properties, int zone) const override
    {
        return {0.0, 0.0, scalar(properties, "external.surface_nuclide_concentration", zone)};
    }
};

}
