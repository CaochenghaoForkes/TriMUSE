#pragma once

#include "properties/external/surface_pressure/surface_pressure_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantSurfacePressure final : public SurfacePressureModel, private ConstantModel
{
public:
    explicit ConstantSurfacePressure(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
