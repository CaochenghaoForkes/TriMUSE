#pragma once
#include "physics/radioactivity/boundary/radioactivity_boundary_model.hpp"
namespace trimuse
{

class RadioactivityRobinBoundary final : public RadioactivityBoundaryModel // j_n=h_m(c-c_infinity)，分别形成左端和右端系数
{
public:
    explicit RadioactivityRobinBoundary(const ModelParameterMap&) {}

    RadioactivityBoundaryValues evaluate(const PropertiesFieldData& properties, int zone) const override
    {
        const double h = scalar(properties, "derived.mass_transfer_coef", zone);
        // 左端 h_m c，右端 h_m c_infinity。
        return {h, h * scalar(properties, "external.environment_nuclide_concentration", zone), 0.0};
    }
};

}
