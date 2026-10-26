#include "properties/derived/heat_transfer_coef/constant_heat_transfer_coef.hpp"

namespace trimuse
{

ConstantHeatTransferCoef::ConstantHeatTransferCoef(const ModelParameterMap& parameters)
    : ConstantModel(parameters) // 初始化基类 ConstantModel 把参数交给公共常数实现
{
}

StateValue ConstantHeatTransferCoef::evaluate(const PropertiesStatePoint& point) const
{
    return ConstantModel::evaluate(point); // 调用基类 ConstantModel 的 evaluate(point)
}

}
