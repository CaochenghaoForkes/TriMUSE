#include "properties/external/nuclide_birth_rate/constant_nuclide_birth_rate.hpp"

namespace trimuse
{

ConstantNuclideBirthRate::ConstantNuclideBirthRate(const ModelParameterMap& parameters, const std::string& value_type)
    : ConstantModel(parameters, value_type)
{
}

StateValue ConstantNuclideBirthRate::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
