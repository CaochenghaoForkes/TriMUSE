#pragma once

#include "properties/derived/face_quadrature_weight/face_quadrature_weight_model_manager.hpp"

namespace trimuse
{

class UndeformedFaceQuadratureWeight final : public FaceQuadratureWeightModel
{
public:
    explicit UndeformedFaceQuadratureWeight(const ModelParameterMap& parameters);
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
