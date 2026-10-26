#include "properties/material/general/mass_density/common/constant_mass_density.hpp"

namespace trimuse
{

ConstantMassDensity::ConstantMassDensity(const ModelParameterMap& parameters)
    : ConstantModel(parameters)
{
}

double ConstantMassDensity::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(ConstantModel::evaluate(point));
}

}
