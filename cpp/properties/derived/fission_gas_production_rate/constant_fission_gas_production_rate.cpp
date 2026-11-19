#include "properties/derived/fission_gas_production_rate/constant_fission_gas_production_rate.hpp"

namespace trimuse
{

ConstantFissionGasProductionRate::ConstantFissionGasProductionRate(const ModelParameterMap& parameters)
    :
    ConstantModel(parameters)
{
}

StateValue ConstantFissionGasProductionRate::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point);
}

}
