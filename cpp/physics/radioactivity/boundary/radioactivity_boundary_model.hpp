#pragma once

#include "common/model_parameters.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

struct RadioactivityBoundaryConfig // 一个边界分区采用的边界模型及其构造参数
{
    int boundary_attribute; // 原网格边界属性编号
    std::string model_name; // dirichlet、neumann 或 robin
    ModelParameterMap parameters; // 模型构造参数
};

struct RadioactivityBoundaryValues // 边界模型统一返回矩阵系数、右端系数和本质边界值
{
    double matrix = 0.0; // Robin 的 h_m
    double load = 0.0; // Robin 的 h_m*c_infinity 或 Neumann 的 -j_n
    double concentration = 0.0; // Dirichlet 指定浓度，atom/m³
};

enum class RadioactivityBoundaryPart { Matrix, Load, Essential }; // 分别跟踪三部分是否变化

class RadioactivityBoundaryModel // 具体边界模型将属性转换成弱式系数或指定浓度
{
public:
    virtual ~RadioactivityBoundaryModel() = default;

    virtual bool essential() const { return false; } // 默认是自然边界；Dirichlet 覆盖为 true

    virtual RadioactivityBoundaryValues evaluate(const PropertiesFieldData& properties, int zone) const = 0; // 按分区属性求边界值

protected:
    static double scalar(const PropertiesFieldData& properties, const std::string& name, int zone) // 读取该边界分区上的标量属性
    {
        return std::get<double>(properties.zone_values.at(name).at(zone).value());
    }
};

}
