#pragma once
#include "physics/thermal/common/thermal_domain_model.hpp"
namespace trimuse
{

class FourierConduction final : public ThermalDomainModel // 具体的傅立叶导热模型 作用是返回指定积分点上的导热系数 k
{
public:
    explicit FourierConduction(const ModelParameterMap&) {} // 构造函数接收参数字典是为了符合注册工厂统一的构造方式

    double evaluate(const PropertiesFieldData& properties, std::size_t point) const override // 对基类纯虚函数 evaluate 的实现
    {
        return scalar(properties, "material.thermal.conductivity", point); // 调用继承来的 scalar 辅助方法，从 material.thermal.conductivity 缓存中读取全域积分点数组中第 point 项对应的该积分点导热系数 k
    }
};

}
