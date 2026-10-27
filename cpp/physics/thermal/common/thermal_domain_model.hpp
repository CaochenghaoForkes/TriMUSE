#pragma once

#include "common/model_parameters.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

struct ThermalDomainConfig // 保存: 某个区域使用哪个热物理模型，以及该模型的参数
{
    int region_attribute; // 只保存 Region 的模型配置
    std::string model_name; // 模型名称
    ModelParameterMap parameters; // 模型参数字典
};

class ThermalDomainModel // 导热、热容、体积热源等域内模型共用的抽象基类，规定这些模型如何读取属性并返回积分点上的系数
{
public:
    virtual ~ThermalDomainModel() = default; // 虚析构函数，采用编译器默认实现

    virtual double evaluate(const PropertiesFieldData& properties, std::size_t point) const = 0; // 所有基体模型必须实现的求值方法 properties: 已经计算缓存好的属性数据 point: 积分点在全域连续数组中的编号(单元偏移量+单元内积分点编号) 返回的 double 是该积分点参与热方程的系数 如导热模型返回 k 热容模型返回 rho*cp 热源模型返回体积功率密度 q

protected:
    static double scalar(const PropertiesFieldData& properties, const std::string& name, std::size_t point) // 供本类和派生类使用的辅助函数，用来从属性缓存中读取一个标量值
    {
        return std::get<double>(properties.integration_point_values.at(name)[point].value()); // 找到指定属性的全域积分数组 -> 找到当前积分点的 optional<StateValue> -> 取出 StateValue -> 从 variant 中取出 double
    }
};

}
