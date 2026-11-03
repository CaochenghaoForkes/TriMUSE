#pragma once

#include "common/model_parameters.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

struct ThermalBoundaryConfig // 某个边界分区采样的边界模型以及传给模型的参数 区域编号、模型名称、构造参数
{
    int boundary_attribute; // 边界属性编号
    std::string model_name; // 模型名称 [dirichlet]['neumann']['robin']
    ModelParameterMap parameters; // 传给模型构造函数的参数字典
};

struct ThermalBoundaryValues // 边界模型求值后，交给组装层使用的系数或指定值 matrix 矩阵项系数 load 右端系数 temperature 一类边界值
{
    double matrix = 0.0; // 边界矩阵项的系数
    double load = 0.0; // 边界右端项的系数
    double temperature = 0.0; // Dirichlet 边界指定的表面温度 用于约束边界自由度
};

enum class BoundaryPart { Matrix, Load, Essential };

class ThermalBoundaryModel
{
public:
    virtual ~ThermalBoundaryModel() = default; // 默认析构

    virtual bool essential() const { return false; } // 判断是否为本质边界条件 默认为 false 组装层通过这个方法决定：该边界需要约束温度自由度，还是提供矩阵、右端项贡献

    virtual ThermalBoundaryValues evaluate(const PropertiesFieldData& properties, int zone) const = 0; // 传入properties与区域编号，返回内容包括边界贡献的矩阵系数(默认0.0)、边界贡献的右端系数(默认0.0)、一类边界值(默认0.0)

protected:
    static double scalar(const PropertiesFieldData& properties, const std::string& name, int zone) // 得到属性表中名为 name 的区域属性在 zone 区域的值
    {
        return std::get<double>(properties.zone_values.at(name).at(zone).value());
    }
};

}
