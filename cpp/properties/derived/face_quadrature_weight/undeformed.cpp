#include "properties/derived/face_quadrature_weight/undeformed.hpp"

namespace trimuse
{

UndeformedFaceQuadratureWeight::UndeformedFaceQuadratureWeight(const ModelParameterMap&)
{
}

StateValue UndeformedFaceQuadratureWeight::evaluate(const PropertiesStatePoint& point) const
{
    return point.scalar("precomputed.initial_face_quadrature_weight");
}

}
