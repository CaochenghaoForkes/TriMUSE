#include "properties/material/general/crystallite_diameter/common/constant_crystallite_diameter.hpp"

namespace trimuse
{

ConstantCrystalliteDiameter::ConstantCrystalliteDiameter(const ModelParameterMap& parameters)
    :
    ConstantModel(parameters)
{
}

double ConstantCrystalliteDiameter::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(ConstantModel::evaluate(point));
}

}
