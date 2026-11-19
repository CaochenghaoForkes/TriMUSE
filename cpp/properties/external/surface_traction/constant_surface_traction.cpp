#include "properties/external/surface_traction/constant_surface_traction.hpp"

namespace trimuse
{

ConstantSurfaceTraction::ConstantSurfaceTraction(const ModelParameterMap& parameters)
    :
    ConstantModel(parameters, "vector")
{
}

VectorValue ConstantSurfaceTraction::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<VectorValue>(ConstantModel::evaluate(point));
}

}
