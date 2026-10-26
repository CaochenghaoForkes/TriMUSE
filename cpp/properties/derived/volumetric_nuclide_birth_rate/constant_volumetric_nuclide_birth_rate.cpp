#include "properties/derived/volumetric_nuclide_birth_rate/constant_volumetric_nuclide_birth_rate.hpp"

namespace trimuse
{

ConstantVolumetricNuclideBirthRate::ConstantVolumetricNuclideBirthRate(const ModelParameterMap& parameters)
    : ConstantModel(parameters)
{
}

StateValue ConstantVolumetricNuclideBirthRate::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
