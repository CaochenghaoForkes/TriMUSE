#pragma once

#include "properties/external/surface_displacement/surface_displacement_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantSurfaceDisplacement final : public SurfaceDisplacementModel, private ConstantModel
{
public:
    explicit ConstantSurfaceDisplacement(const ModelParameterMap& parameters);

    VectorValue evaluate(const PropertiesStatePoint& point) const override;
};

}
