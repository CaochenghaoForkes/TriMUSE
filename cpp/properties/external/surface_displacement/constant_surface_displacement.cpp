#include "properties/external/surface_displacement/constant_surface_displacement.hpp"

namespace trimuse
{

ConstantSurfaceDisplacement::ConstantSurfaceDisplacement(const ModelParameterMap& parameters)
    :
    ConstantModel(parameters, "vector")
{
}

VectorValue ConstantSurfaceDisplacement::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<VectorValue>(ConstantModel::evaluate(point));
}

}
