#include "properties/derived/volumetric_power/constant_volumetric_power.hpp"

namespace trimuse
{

ConstantVolumetricPower::ConstantVolumetricPower(const ModelParameterMap& parameters)
    : ConstantModel(parameters)
{
}

StateValue ConstantVolumetricPower::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
