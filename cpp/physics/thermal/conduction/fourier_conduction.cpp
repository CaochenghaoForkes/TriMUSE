#include "physics/thermal/conduction/fourier_conduction.hpp"

namespace trimuse
{

FourierConduction::FourierConduction(const ModelParameterMap&)
{
}

double FourierConduction::evaluate(const PropertiesFieldData& properties, std::size_t point) const // 对基类纯虚函数 evaluate 的实现
{
    return scalar(properties, "material.thermal.conductivity", point); // 调用继承来的 scalar 辅助方法，从 material.thermal.conductivity 缓存中读取全域积分点数组中第 point 项对应的该积分点导热系数 k
}

}
