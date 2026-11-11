#include "properties/material/general/bacon_anisotropy_factor/common/constant_bacon_anisotropy_factor.hpp"

namespace trimuse
{

ConstantBaconAnisotropyFactor::ConstantBaconAnisotropyFactor(const ModelParameterMap& parameters)
    : ConstantModel(parameters)
{
}

double ConstantBaconAnisotropyFactor::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(ConstantModel::evaluate(point));
}

}
