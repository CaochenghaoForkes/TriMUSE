#pragma once
#include "physics/radioactivity/boundary/radioactivity_boundary_model.hpp"

namespace trimuse
{

class RadioactivityNeumannBoundary final : public RadioactivityBoundaryModel // 指定外向核素通量，只向右端向量贡献载荷
{
public:
    explicit RadioactivityNeumannBoundary(const ModelParameterMap&);
    RadioactivityBoundaryValues evaluate(const PropertiesFieldData& properties, int zone) const override;
};

}
