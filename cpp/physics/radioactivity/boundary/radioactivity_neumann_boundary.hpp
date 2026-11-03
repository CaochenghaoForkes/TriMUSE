#pragma once
#include "physics/radioactivity/boundary/radioactivity_boundary_model.hpp"
namespace trimuse
{

class RadioactivityNeumannBoundary final : public RadioactivityBoundaryModel // 指定外向核素通量，只向右端向量贡献载荷
{
public:
    explicit RadioactivityNeumannBoundary(const ModelParameterMap&) {}

    RadioactivityBoundaryValues evaluate(const PropertiesFieldData& properties, int zone) const override
    {
        // j_n = -D grad(c) · n, 外流为正; 弱形式右端系数为 -j_n。
        return {0.0, -scalar(properties, "external.nuclide_flux", zone), 0.0};
    }
};

}
