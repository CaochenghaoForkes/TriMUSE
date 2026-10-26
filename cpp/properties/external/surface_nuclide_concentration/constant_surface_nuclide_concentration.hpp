#pragma once

#include "properties/external/surface_nuclide_concentration/surface_nuclide_concentration_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantSurfaceNuclideConcentration final : public SurfaceNuclideConcentrationModel, private ConstantModel
{
public:
    explicit ConstantSurfaceNuclideConcentration(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
