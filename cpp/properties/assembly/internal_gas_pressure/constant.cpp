#include "properties/assembly/internal_gas_pressure/constant.hpp"

namespace trimuse
{

ConstantInternalGasPressure::ConstantInternalGasPressure(const ModelParameterMap& parameters)
    :
    ConstantModel(parameters)
{
}

double ConstantInternalGasPressure::evaluate(const PropertiesStatePoint& point) const
{
    return std::get<double>(ConstantModel::evaluate(point));
}

}
