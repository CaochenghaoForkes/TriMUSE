#include "properties/derived/heat_transfer_coef/constant.hpp"

namespace trimuse
{

ConstantHeatTransferCoef::ConstantHeatTransferCoef(const ModelParameterMap& parameters)
    : value_(std::get<double>(parameters.at("constant_value")))
{
}

StateValue ConstantHeatTransferCoef::evaluate(const PropertiesStatePoint&) const
{
    return value_;
}

}
