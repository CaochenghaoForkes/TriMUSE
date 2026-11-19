#include "properties/derived/average_volumetric_fission_rate/constant_average_volumetric_fission_rate.hpp"

namespace trimuse
{

ConstantAverageVolumetricFissionRate::ConstantAverageVolumetricFissionRate(const ModelParameterMap& parameters)
    :
    ConstantModel(parameters)
{
}

StateValue ConstantAverageVolumetricFissionRate::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
