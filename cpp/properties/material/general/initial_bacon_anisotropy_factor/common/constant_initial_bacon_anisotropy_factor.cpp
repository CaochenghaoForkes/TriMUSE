#include "properties/material/general/initial_bacon_anisotropy_factor/common/constant_initial_bacon_anisotropy_factor.hpp"

namespace trimuse
{

ConstantInitialBaconAnisotropyFactor::ConstantInitialBaconAnisotropyFactor(const ModelParameterMap& parameters)
    :
    ConstantModel(parameters)
{
}

double ConstantInitialBaconAnisotropyFactor::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(ConstantModel::evaluate(point));
}

}
