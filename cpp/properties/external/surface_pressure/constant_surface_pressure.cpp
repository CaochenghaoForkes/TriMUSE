#include "properties/external/surface_pressure/constant_surface_pressure.hpp"

namespace trimuse
{

ConstantSurfacePressure::ConstantSurfacePressure(const ModelParameterMap& parameters)
    : ConstantModel(parameters, "scalar")
{
}

double ConstantSurfacePressure::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(ConstantModel::evaluate(point));
}

}
