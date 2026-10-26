#include "properties/external/surface_temperature/constant_surface_temperature.hpp"

namespace trimuse
{

ConstantSurfaceTemperature::ConstantSurfaceTemperature(const ModelParameterMap& parameters, const std::string& value_type)
    : ConstantModel(parameters, value_type)
{
}

StateValue ConstantSurfaceTemperature::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
