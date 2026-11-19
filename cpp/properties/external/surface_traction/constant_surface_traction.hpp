#pragma once

#include "properties/external/surface_traction/surface_traction_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantSurfaceTraction final : public SurfaceTractionModel, private ConstantModel
{
public:
    explicit ConstantSurfaceTraction(const ModelParameterMap& parameters);
    VectorValue evaluate(const PropertiesStatePoint& point) const override;
};

}
