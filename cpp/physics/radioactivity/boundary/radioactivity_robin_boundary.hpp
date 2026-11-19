#pragma once
#include "physics/radioactivity/boundary/radioactivity_boundary_model.hpp"

namespace trimuse
{

class RadioactivityRobinBoundary final : public RadioactivityBoundaryModel // j_n=h_m(c-c_infinity)，分别形成左端和右端系数
{
public:
    explicit RadioactivityRobinBoundary(const ModelParameterMap&);
    RadioactivityBoundaryValues evaluate(const PropertiesFieldData& properties, int zone) const override;
};

}
